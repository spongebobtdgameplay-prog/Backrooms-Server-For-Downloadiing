#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

enum class UpdateStage
{
    Idle,
    Checking,
    UpToDate,
    UpdateAvailable,
    Downloading,
    ReadyToApply,
    Failed
};

struct UpdateVisualState
{
    UpdateStage Stage = UpdateStage::Idle;

    std::string CurrentVersion;
    std::string RemoteVersion;

    std::string Headline;
    std::string Message;
    std::string PackageName;
    std::string ReleaseNotes;

    float Progress = 0.0f;

    std::uint64_t DownloadedBytes = 0;
    std::uint64_t TotalBytes = 0;
};

class UpdaterService
{
public:
    UpdaterService() = default;
    ~UpdaterService();

    UpdaterService(const UpdaterService&) = delete;
    UpdaterService& operator=(const UpdaterService&) = delete;

    void Initialize(
        const std::filesystem::path& InstallDirectory,
        const std::wstring& ManifestUrl,
        const std::string& CurrentVersion
    );

    void Shutdown();

    void BeginCheck();
    void BeginDownload();

    UpdateStage Stage() const;
    UpdateVisualState VisualState() const;

    bool ShouldBlockGame() const;
    bool HasUpdate() const;
    bool ReadyToApply() const;
    bool Failed() const;

    bool LaunchApplyAndRestart() const;

private:
    void JoinWorker();
    void CheckWorker();
    void DownloadWorker();

    bool DownloadText(
        const std::wstring& Url,
        std::string& OutText
    ) const;

    bool DownloadPackage(
        const std::wstring& Url,
        const std::filesystem::path& Destination
    );

    bool ParseManifest(const std::string& Text);
    bool VerifyDownloadedPackage() const;
    bool WriteApplyScript();

    void SetFailure(
        const std::string& Message
    );

    static int ParseVersion(
        const std::string& Version
    );

    static std::string Trim(
        const std::string& Text
    );

    static std::wstring Utf8ToWide(
        const std::string& Text
    );

    static std::string Sha256File(
        const std::filesystem::path& Path
    );

    static std::string PowerShellLiteral(
        const std::filesystem::path& Path
    );

    std::filesystem::path InstallDirectory;
    std::filesystem::path TempDirectory;
    std::filesystem::path PendingPackage;
    std::filesystem::path ApplyScript;

    std::wstring ManifestUrl;

    mutable std::mutex Mutex;
    std::thread Worker;

    UpdateStage CurrentStage = UpdateStage::Idle;

    std::string CurrentVersion;
    std::string RemoteVersion;
    std::string PackageName;
    std::wstring PackageUrl;
    std::string PackageSha256;
    std::string ReleaseNotes;

    std::string Headline;
    std::string Message;

    std::uint64_t DownloadedBytes = 0;
    std::uint64_t TotalBytes = 0;
    float Progress = 0.0f;
};
