#pragma once

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
};

class UpdaterService
{
public:
    UpdaterService() = default;
    ~UpdaterService();

    UpdaterService(const UpdaterService&) = delete;
    UpdaterService& operator=(const UpdaterService&) = delete;

    void Initialize(
        const std::wstring& ManifestUrl,
        const std::string& CurrentVersion
    );

    void Shutdown();
    void BeginCheck();

    UpdateStage Stage() const;
    UpdateVisualState VisualState() const;

    bool ShouldBlockGame() const;
    bool HasUpdate() const;
    bool Failed() const;

    std::string DownloadUrl() const;

private:
    void JoinWorker();
    void CheckWorker();

    bool DownloadText(
        const std::wstring& Url,
        std::string& OutText
    ) const;

    bool ParseManifest(const std::string& Text);

    void SetFailure(
        const std::string& Message
    );

    static int ParseVersion(
        const std::string& Version
    );

    static std::string Trim(
        const std::string& Text
    );

    std::wstring ManifestUrl;

    mutable std::mutex Mutex;
    std::thread Worker;

    UpdateStage CurrentStage = UpdateStage::Idle;

    std::string CurrentVersion;
    std::string RemoteVersion;
    std::string PackageName;
    std::string PackageUrl;
    std::string ReleaseNotes;

    std::string Headline;
    std::string Message;
};
