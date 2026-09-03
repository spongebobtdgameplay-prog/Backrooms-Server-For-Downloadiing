#include "UpdaterService.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
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
        Secure =
            Parts.nScheme ==
            INTERNET_SCHEME_HTTPS;

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
            L"BackroomsOfficalUpdateCheck/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (Session.Value == nullptr)
            return false;

        WinHttpSetTimeouts(
            Session.Value,
            4000,
            4000,
            8000,
            8000
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

        return
            StatusCode >= 200 &&
            StatusCode < 300;
    }
}
#endif

UpdaterService::~UpdaterService()
{
    Shutdown();
}

void UpdaterService::Initialize(
    const std::wstring& NewManifestUrl,
    const std::string& NewCurrentVersion
)
{
    ManifestUrl = NewManifestUrl;
    CurrentVersion = NewCurrentVersion;
}

void UpdaterService::Shutdown()
{
    JoinWorker();
}

void UpdaterService::JoinWorker()
{
    if (Worker.joinable())
        Worker.join();
}

void UpdaterService::BeginCheck()
{
    JoinWorker();

    {
        std::scoped_lock Lock(Mutex);

        CurrentStage = UpdateStage::Checking;

        RemoteVersion.clear();
        PackageName.clear();
        PackageUrl.clear();
        ReleaseNotes.clear();

        Headline = "CHECKING FOR UPDATES";
        Message = "CONTACTING THE UPDATE SERVER";
    }

    Worker =
        std::thread(
            &UpdaterService::CheckWorker,
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

    return State;
}

bool UpdaterService::ShouldBlockGame() const
{
    const UpdateStage Value = Stage();

    return
        Value == UpdateStage::Checking ||
        Value == UpdateStage::UpdateAvailable ||
        Value == UpdateStage::Failed;
}

bool UpdaterService::HasUpdate() const
{
    return
        Stage() ==
        UpdateStage::UpdateAvailable;
}

bool UpdaterService::Failed() const
{
    return
        Stage() ==
        UpdateStage::Failed;
}

std::string UpdaterService::DownloadUrl() const
{
    std::scoped_lock Lock(Mutex);
    return PackageUrl;
}

void UpdaterService::CheckWorker()
{
#ifdef _WIN32
    std::string Manifest;

    if (!DownloadText(ManifestUrl, Manifest))
    {
        SetFailure(
            "COULD NOT REACH THE UPDATE SERVER"
        );
        return;
    }

    if (!ParseManifest(Manifest))
    {
        SetFailure(
            "THE UPDATE MANIFEST IS INVALID"
        );
        return;
    }

    const int Local =
        ParseVersion(CurrentVersion);

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
        Headline = "UPDATE AVAILABLE";
        Message = "PRESS ENTER TO GET THE NEW BUILD";
    }
#else
    std::scoped_lock Lock(Mutex);
    CurrentStage = UpdateStage::UpToDate;
    Headline = "UP TO DATE";
    Message = "UPDATE CHECK IS WINDOWS ONLY";
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

    if (
        !OpenGetRequest(
            Url,
            Session,
            Connection,
            Request
        )
    )
    {
        return false;
    }

    std::string Result;

    while (true)
    {
        DWORD Available = 0;

        if (
            !WinHttpQueryDataAvailable(
                Request.Value,
                &Available
            )
        )
        {
            return false;
        }

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

    OutText = std::move(Result);
    return !OutText.empty();
#else
    static_cast<void>(Url);
    static_cast<void>(OutText);
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
    std::string Notes;

    std::string Line;

    while (std::getline(Stream, Line))
    {
        Line = Trim(Line);

        if (
            Line.empty() ||
            Line[0] == '#'
        )
        {
            continue;
        }

        const std::size_t Equals =
            Line.find('=');

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
        else if (Key == "notes")
            Notes = Value;
    }

    if (
        Version.empty() ||
        Name.empty() ||
        Url.empty()
    )
    {
        return false;
    }

    {
        std::scoped_lock Lock(Mutex);

        RemoteVersion = Version;
        PackageName = Name;
        PackageUrl = Url;
        ReleaseNotes = Notes;
    }

    return true;
}

void UpdaterService::SetFailure(
    const std::string& FailureMessage
)
{
    std::scoped_lock Lock(Mutex);

    CurrentStage = UpdateStage::Failed;
    Headline = "UPDATE CHECK FAILED";
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
