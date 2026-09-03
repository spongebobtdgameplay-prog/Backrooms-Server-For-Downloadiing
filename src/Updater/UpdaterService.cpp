#include "UpdaterService.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <winhttp.h>

namespace
{
    struct InternetHandle
    {
        HINTERNET Value = nullptr;

        ~InternetHandle()
        {
            if (Value != nullptr)
                WinHttpCloseHandle(Value);
        }
    };

    bool SplitUrl(
        const std::wstring& Url,
        std::wstring& Host,
        std::wstring& Path,
        INTERNET_PORT& Port,
        bool& Secure
    )
    {
        URL_COMPONENTS Parts{};
        Parts.dwStructSize = sizeof(Parts);
        Parts.dwSchemeLength = static_cast<DWORD>(-1);
        Parts.dwHostNameLength = static_cast<DWORD>(-1);
        Parts.dwUrlPathLength = static_cast<DWORD>(-1);
        Parts.dwExtraInfoLength = static_cast<DWORD>(-1);

        if (!WinHttpCrackUrl(Url.c_str(), 0, 0, &Parts))
            return false;

        Host.assign(
            Parts.lpszHostName,
            Parts.dwHostNameLength
        );

        Path.assign(
            Parts.lpszUrlPath,
            Parts.dwUrlPathLength
        );

        if (
            Parts.lpszExtraInfo != nullptr &&
            Parts.dwExtraInfoLength > 0
        )
        {
            Path.append(
                Parts.lpszExtraInfo,
                Parts.dwExtraInfoLength
            );
        }

        Port = Parts.nPort;
        Secure = Parts.nScheme == INTERNET_SCHEME_HTTPS;
        return true;
    }

    bool OpenGetRequest(
        const std::wstring& Url,
        InternetHandle& Session,
        InternetHandle& Connection,
        InternetHandle& Request
    )
    {
        std::wstring Host;
        std::wstring Path;
        INTERNET_PORT Port = 0;
        bool Secure = false;

        if (!SplitUrl(Url, Host, Path, Port, Secure))
            return false;

        Session.Value = WinHttpOpen(
            L"BackroomsOfficalUpdater/2.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (Session.Value == nullptr)
            return false;

        WinHttpSetTimeouts(
            Session.Value,
            5000,
            5000,
            15000,
            15000
        );

        Connection.Value = WinHttpConnect(
            Session.Value,
            Host.c_str(),
            Port,
            0
        );

        if (Connection.Value == nullptr)
            return false;

        Request.Value = WinHttpOpenRequest(
            Connection.Value,
            L"GET",
            Path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            Secure ? WINHTTP_FLAG_SECURE : 0
        );

        if (Request.Value == nullptr)
            return false;

        const wchar_t* Headers =
            L"Cache-Control: no-cache\r\n"
            L"Pragma: no-cache\r\n";

        if (
            !WinHttpSendRequest(
                Request.Value,
                Headers,
                static_cast<DWORD>(-1),
                WINHTTP_NO_REQUEST_DATA,
                0,
                0,
                0
            )
        )
        {
            return false;
        }

        if (!WinHttpReceiveResponse(Request.Value, nullptr))
            return false;

        DWORD StatusCode = 0;
        DWORD Size = sizeof(StatusCode);

        if (
            !WinHttpQueryHeaders(
                Request.Value,
                WINHTTP_QUERY_STATUS_CODE |
                    WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &StatusCode,
                &Size,
                WINHTTP_NO_HEADER_INDEX
            )
        )
        {
            return false;
        }

        return StatusCode >= 200 && StatusCode < 300;
    }

    std::uint64_t ContentLength(HINTERNET Request)
    {
        wchar_t Buffer[64] = {};
        DWORD Size = sizeof(Buffer);

        if (
            !WinHttpQueryHeaders(
                Request,
                WINHTTP_QUERY_CONTENT_LENGTH,
                WINHTTP_HEADER_NAME_BY_INDEX,
                Buffer,
                &Size,
                WINHTTP_NO_HEADER_INDEX
            )
        )
        {
            return 0;
        }

        try
        {
            return std::stoull(Buffer);
        }
        catch (...)
        {
            return 0;
        }
    }
}
#endif

UpdaterService::~UpdaterService()
{
    Shutdown();
}

void UpdaterService::Initialize(
    const std::filesystem::path& NewInstallDirectory,
    const std::wstring& NewManifestUrl,
    const std::string& NewCurrentVersion
)
{
    InstallDirectory = NewInstallDirectory;
    ManifestUrl = NewManifestUrl;
    CurrentVersion = NewCurrentVersion;

    TempDirectory =
        std::filesystem::temp_directory_path() /
        "BackroomsOfficalUpdate";
}

void UpdaterService::Shutdown()
{
    CancelRequested.store(true);
    JoinWorker();
}

void UpdaterService::JoinWorker()
{
    if (Worker.joinable())
        Worker.join();
}

void UpdaterService::BeginCheck()
{
    CancelRequested.store(true);
    JoinWorker();
    CancelRequested.store(false);

    {
        std::scoped_lock Lock(Mutex);

        CurrentStage = UpdateStage::Checking;

        RemoteVersion.clear();
        PackageName.clear();
        PackageUrl.clear();
        PackageSha256.clear();
        ReleaseNotes.clear();

        Headline = "CHECKING FOR UPDATES";
        Message = "CONTACTING UPDATE SERVER";

        Progress = 0.0f;
        DownloadedBytes = 0;
        TotalBytes = 0;
    }

    Worker = std::thread(
        &UpdaterService::CheckWorker,
        this
    );
}

void UpdaterService::BeginDownload()
{
    CancelRequested.store(true);
    JoinWorker();
    CancelRequested.store(false);

    {
        std::scoped_lock Lock(Mutex);

        if (CurrentStage != UpdateStage::UpdateAvailable)
            return;

        CurrentStage = UpdateStage::Downloading;
        Headline = "UPDATE NEEDED";
        Message = "DOWNLOADING THE NEW BUILD";
        Progress = 0.0f;
        DownloadedBytes = 0;
        TotalBytes = 0;
    }

    Worker = std::thread(
        &UpdaterService::DownloadWorker,
        this
    );
}

UpdateStage UpdaterService::Stage() const
{
    std::scoped_lock Lock(Mutex);
    return CurrentStage;
}

UpdateVisualState UpdaterService::VisualState() const
{
    std::scoped_lock Lock(Mutex);

    UpdateVisualState State;
    State.Stage = CurrentStage;
    State.CurrentVersion = CurrentVersion;
    State.RemoteVersion = RemoteVersion;
    State.Headline = Headline;
    State.Message = Message;
    State.PackageName = PackageName;
    State.ReleaseNotes = ReleaseNotes;
    State.Progress = Progress;
    State.DownloadedBytes = DownloadedBytes;
    State.TotalBytes = TotalBytes;

    return State;
}

bool UpdaterService::ShouldBlockGame() const
{
    return Stage() != UpdateStage::Idle &&
        Stage() != UpdateStage::UpToDate;
}

bool UpdaterService::HasUpdate() const
{
    return Stage() == UpdateStage::UpdateAvailable;
}

bool UpdaterService::ReadyToInstall() const
{
    return Stage() == UpdateStage::ReadyToInstall;
}

bool UpdaterService::Failed() const
{
    return Stage() == UpdateStage::Failed;
}

void UpdaterService::CheckWorker()
{
#ifdef _WIN32
    std::string Manifest;

    if (!DownloadText(ManifestUrl, Manifest))
    {
        if (!CancelRequested.load())
            SetFailure("COULD NOT REACH THE UPDATE SERVER");
        return;
    }

    if (!ParseManifest(Manifest))
    {
        if (!CancelRequested.load())
            SetFailure("THE UPDATE MANIFEST IS INVALID");
        return;
    }

    const int Local = ParseVersion(CurrentVersion);

    int Remote = 0;

    {
        std::scoped_lock Lock(Mutex);
        Remote = ParseVersion(RemoteVersion);
    }

    if (Remote <= Local)
    {
        std::scoped_lock Lock(Mutex);
        CurrentStage = UpdateStage::UpToDate;
        Headline = "UP TO DATE";
        Message = "YOUR BUILD IS CURRENT";
        return;
    }

    {
        std::scoped_lock Lock(Mutex);
        CurrentStage = UpdateStage::UpdateAvailable;
        Headline = "UPDATE NEEDED";
        Message = "YOUR VERSION IS OUTDATED";
    }
#else
    std::scoped_lock Lock(Mutex);
    CurrentStage = UpdateStage::UpToDate;
    Headline = "UP TO DATE";
    Message = "UPDATE CHECK IS WINDOWS ONLY";
#endif
}

void UpdaterService::DownloadWorker()
{
#ifdef _WIN32
    std::wstring Url;
    std::string Name;

    {
        std::scoped_lock Lock(Mutex);
        Url = PackageUrl;
        Name = PackageName;
    }

    if (Url.empty() || Name.empty())
    {
        SetFailure("UPDATE PACKAGE INFORMATION IS MISSING");
        return;
    }

    std::error_code Error;
    std::filesystem::create_directories(
        TempDirectory,
        Error
    );

    if (Error)
    {
        SetFailure("COULD NOT PREPARE UPDATE STORAGE");
        return;
    }

    PendingPackage =
        TempDirectory /
        std::filesystem::path(Name);

    std::filesystem::remove(PendingPackage, Error);

    if (!DownloadPackage(Url, PendingPackage))
    {
        if (!CancelRequested.load())
            SetFailure("DOWNLOAD FAILED  PRESS ENTER TO RETRY");
        return;
    }

    if (CancelRequested.load())
        return;

    {
        std::scoped_lock Lock(Mutex);
        CurrentStage = UpdateStage::Verifying;
        Headline = "UPDATE NEEDED";
        Message = "VERIFYING DOWNLOAD";
        Progress = 1.0f;
    }

    if (!VerifyDownloadedPackage())
    {
        SetFailure("DOWNLOAD VERIFICATION FAILED");
        return;
    }

    {
        std::scoped_lock Lock(Mutex);
        CurrentStage = UpdateStage::ReadyToInstall;
        Headline = "UPDATE READY";
        Message = "INSTALLS INTO YOUR CURRENT GAME FOLDER";
        Progress = 1.0f;
    }
#else
    SetFailure("AUTOMATIC UPDATES ARE WINDOWS ONLY");
#endif
}

bool UpdaterService::DownloadText(
    const std::wstring& Url,
    std::string& OutText
) const
{
#ifdef _WIN32
    InternetHandle Session;
    InternetHandle Connection;
    InternetHandle Request;

    if (!OpenGetRequest(Url, Session, Connection, Request))
        return false;

    std::string Result;

    while (!CancelRequested.load())
    {
        DWORD Available = 0;

        if (!WinHttpQueryDataAvailable(Request.Value, &Available))
            return false;

        if (Available == 0)
            break;

        std::string Buffer;
        Buffer.resize(Available);

        DWORD Read = 0;

        if (
            !WinHttpReadData(
                Request.Value,
                Buffer.data(),
                Available,
                &Read
            )
        )
        {
            return false;
        }

        Buffer.resize(Read);
        Result += Buffer;
    }

    if (CancelRequested.load())
        return false;

    OutText = std::move(Result);
    return !OutText.empty();
#else
    static_cast<void>(Url);
    static_cast<void>(OutText);
    return false;
#endif
}

bool UpdaterService::DownloadPackage(
    const std::wstring& Url,
    const std::filesystem::path& Destination
)
{
#ifdef _WIN32
    InternetHandle Session;
    InternetHandle Connection;
    InternetHandle Request;

    if (!OpenGetRequest(Url, Session, Connection, Request))
        return false;

    const std::uint64_t Expected =
        ContentLength(Request.Value);

    {
        std::scoped_lock Lock(Mutex);
        TotalBytes = Expected;
    }

    std::ofstream Output(
        Destination,
        std::ios::binary |
        std::ios::trunc
    );

    if (!Output.is_open())
        return false;

    std::uint64_t Downloaded = 0;

    while (!CancelRequested.load())
    {
        DWORD Available = 0;

        if (!WinHttpQueryDataAvailable(Request.Value, &Available))
            return false;

        if (Available == 0)
            break;

        std::vector<char> Buffer(
            static_cast<std::size_t>(Available)
        );

        DWORD Read = 0;

        if (
            !WinHttpReadData(
                Request.Value,
                Buffer.data(),
                Available,
                &Read
            )
        )
        {
            return false;
        }

        if (Read == 0)
            break;

        Output.write(
            Buffer.data(),
            static_cast<std::streamsize>(Read)
        );

        if (!Output.good())
            return false;

        Downloaded += Read;

        {
            std::scoped_lock Lock(Mutex);
            DownloadedBytes = Downloaded;

            if (Expected > 0)
            {
                Progress = std::clamp(
                    static_cast<float>(
                        static_cast<double>(Downloaded) /
                        static_cast<double>(Expected)
                    ),
                    0.0f,
                    1.0f
                );
            }
        }
    }

    Output.close();

    if (CancelRequested.load())
        return false;

    if (Downloaded == 0)
        return false;

    if (Expected > 0 && Downloaded != Expected)
        return false;

    return true;
#else
    static_cast<void>(Url);
    static_cast<void>(Destination);
    return false;
#endif
}

bool UpdaterService::ParseManifest(
    const std::string& Text
)
{
    std::istringstream Stream(Text);

    std::string Version;
    std::string Name;
    std::string Url;
    std::string Sha;
    std::string Notes;

    std::string Line;

    while (std::getline(Stream, Line))
    {
        Line = Trim(Line);

        if (Line.empty() || Line[0] == '#')
            continue;

        const std::size_t Equals = Line.find('=');

        if (Equals == std::string::npos)
            continue;

        const std::string Key =
            Trim(Line.substr(0, Equals));

        const std::string Value =
            Trim(Line.substr(Equals + 1));

        if (Key == "version")
            Version = Value;
        else if (Key == "package")
            Name = Value;
        else if (Key == "url")
            Url = Value;
        else if (Key == "sha256")
            Sha = Value;
        else if (Key == "notes")
            Notes = Value;
    }

    if (
        Version.empty() ||
        Name.empty() ||
        Url.empty() ||
        Sha.empty()
    )
    {
        return false;
    }

    {
        std::scoped_lock Lock(Mutex);
        RemoteVersion = Version;
        PackageName = Name;
        PackageUrl = Utf8ToWide(Url);
        PackageSha256 = Sha;
        ReleaseNotes = Notes;
    }

    return true;
}

bool UpdaterService::VerifyDownloadedPackage() const
{
    std::string Expected;

    {
        std::scoped_lock Lock(Mutex);
        Expected = PackageSha256;
    }

    const std::string Actual =
        Sha256File(PendingPackage);

    if (Actual.empty())
        return false;

    auto Normalize = [](std::string Value)
    {
        std::transform(
            Value.begin(),
            Value.end(),
            Value.begin(),
            [](unsigned char Character)
            {
                return static_cast<char>(
                    std::tolower(Character)
                );
            }
        );

        return Value;
    };

    return Normalize(Actual) == Normalize(Expected);
}

bool UpdaterService::LaunchInstaller() const
{
#ifdef _WIN32
    if (Stage() != UpdateStage::ReadyToInstall)
        return false;

    if (!std::filesystem::exists(PendingPackage))
        return false;

    const std::wstring Directory =
        InstallDirectory.wstring();

    const std::wstring Parameters =
        L"/VERYSILENT "
        L"/SUPPRESSMSGBOXES "
        L"/NORESTART "
        L"/CLOSEAPPLICATIONS "
        L"/DIR=\"" +
        Directory +
        L"\"";

    const HINSTANCE Result = ShellExecuteW(
        nullptr,
        L"open",
        PendingPackage.wstring().c_str(),
        Parameters.c_str(),
        TempDirectory.wstring().c_str(),
        SW_SHOWNORMAL
    );

    return reinterpret_cast<std::intptr_t>(Result) > 32;
#else
    return false;
#endif
}

void UpdaterService::SetFailure(
    const std::string& FailureMessage
)
{
    std::scoped_lock Lock(Mutex);
    CurrentStage = UpdateStage::Failed;
    Headline = "UPDATE FAILED";
    Message = FailureMessage;
}

int UpdaterService::ParseVersion(
    const std::string& Version
)
{
    int Major = 0;
    int Minor = 0;
    int Patch = 0;

    if (
        std::sscanf(
            Version.c_str(),
            "%d.%d.%d",
            &Major,
            &Minor,
            &Patch
        ) < 1
    )
    {
        return 0;
    }

    return
        Major * 1000000 +
        Minor * 1000 +
        Patch;
}

std::string UpdaterService::Trim(
    const std::string& Text
)
{
    const auto NotSpace =
        [](unsigned char Character)
        {
            return !std::isspace(Character);
        };

    const auto Begin =
        std::find_if(
            Text.begin(),
            Text.end(),
            NotSpace
        );

    if (Begin == Text.end())
        return {};

    const auto End =
        std::find_if(
            Text.rbegin(),
            Text.rend(),
            NotSpace
        ).base();

    return std::string(Begin, End);
}

std::wstring UpdaterService::Utf8ToWide(
    const std::string& Text
)
{
#ifdef _WIN32
    if (Text.empty())
        return {};

    const int Count = MultiByteToWideChar(
        CP_UTF8,
        0,
        Text.data(),
        static_cast<int>(Text.size()),
        nullptr,
        0
    );

    if (Count <= 0)
        return {};

    std::wstring Result;
    Result.resize(static_cast<std::size_t>(Count));

    MultiByteToWideChar(
        CP_UTF8,
        0,
        Text.data(),
        static_cast<int>(Text.size()),
        Result.data(),
        Count
    );

    return Result;
#else
    return std::wstring(Text.begin(), Text.end());
#endif
}

std::string UpdaterService::Sha256File(
    const std::filesystem::path& Path
)
{
#ifdef _WIN32
    std::ifstream File(Path, std::ios::binary);

    if (!File.is_open())
        return {};

    BCRYPT_ALG_HANDLE Algorithm = nullptr;
    BCRYPT_HASH_HANDLE Hash = nullptr;

    DWORD ObjectLength = 0;
    DWORD HashLength = 0;
    DWORD ResultLength = 0;

    if (
        BCryptOpenAlgorithmProvider(
            &Algorithm,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0
        ) < 0
    )
    {
        return {};
    }

    auto Cleanup = [&]()
    {
        if (Hash != nullptr)
            BCryptDestroyHash(Hash);

        if (Algorithm != nullptr)
            BCryptCloseAlgorithmProvider(Algorithm, 0);
    };

    if (
        BCryptGetProperty(
            Algorithm,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&ObjectLength),
            sizeof(ObjectLength),
            &ResultLength,
            0
        ) < 0 ||
        BCryptGetProperty(
            Algorithm,
            BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&HashLength),
            sizeof(HashLength),
            &ResultLength,
            0
        ) < 0
    )
    {
        Cleanup();
        return {};
    }

    std::vector<unsigned char> HashObject(ObjectLength);
    std::vector<unsigned char> Digest(HashLength);

    if (
        BCryptCreateHash(
            Algorithm,
            &Hash,
            HashObject.data(),
            ObjectLength,
            nullptr,
            0,
            0
        ) < 0
    )
    {
        Cleanup();
        return {};
    }

    std::vector<unsigned char> Buffer(64 * 1024);

    while (File.good())
    {
        File.read(
            reinterpret_cast<char*>(Buffer.data()),
            static_cast<std::streamsize>(Buffer.size())
        );

        const std::streamsize Count = File.gcount();

        if (Count <= 0)
            break;

        if (
            BCryptHashData(
                Hash,
                Buffer.data(),
                static_cast<ULONG>(Count),
                0
            ) < 0
        )
        {
            Cleanup();
            return {};
        }
    }

    if (
        BCryptFinishHash(
            Hash,
            Digest.data(),
            HashLength,
            0
        ) < 0
    )
    {
        Cleanup();
        return {};
    }

    Cleanup();

    std::ostringstream Stream;
    Stream << std::hex << std::setfill('0');

    for (unsigned char Byte : Digest)
    {
        Stream
            << std::setw(2)
            << static_cast<int>(Byte);
    }

    return Stream.str();
#else
    static_cast<void>(Path);
    return {};
#endif
}
