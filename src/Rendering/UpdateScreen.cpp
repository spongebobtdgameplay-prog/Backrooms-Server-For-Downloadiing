#include "Renderer.h"

#include "../Updater/UpdaterService.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string SanitizeUpdateText(
        const std::string& Text,
        std::size_t MaxLength = 0
    )
    {
        std::string Result;
        Result.reserve(Text.size());

        bool PreviousSpace = false;

        for (unsigned char Raw : Text)
        {
            char Character = static_cast<char>(std::toupper(Raw));

            const bool Allowed =
                (Character >= 'A' && Character <= 'Z') ||
                (Character >= '0' && Character <= '9') ||
                Character == ' ' ||
                Character == '.' ||
                Character == '/' ||
                Character == '-' ||
                Character == ':' ||
                Character == '%';

            if (!Allowed)
                Character = ' ';

            if (Character == ' ')
            {
                if (PreviousSpace)
                    continue;

                PreviousSpace = true;
            }
            else
            {
                PreviousSpace = false;
            }

            Result.push_back(Character);

            if (
                MaxLength > 0 &&
                Result.size() >= MaxLength
            )
            {
                break;
            }
        }

        while (!Result.empty() && Result.back() == ' ')
            Result.pop_back();

        return Result;
    }

    std::string FormatMegabytes(std::uint64_t Bytes)
    {
        const double Megabytes =
            static_cast<double>(Bytes) /
            (1024.0 * 1024.0);

        std::ostringstream Stream;
        Stream
            << std::fixed
            << std::setprecision(1)
            << Megabytes
            << " MB";

        return Stream.str();
    }

    std::vector<std::string> WrapUpdateText(
        const std::string& Text,
        std::size_t MaximumCharacters,
        std::size_t MaximumLines
    )
    {
        std::vector<std::string> Lines;
        std::istringstream Stream(
            SanitizeUpdateText(Text)
        );

        std::string Word;
        std::string Current;

        while (Stream >> Word)
        {
            const std::size_t CandidateLength =
                Current.empty()
                    ? Word.size()
                    : Current.size() + 1 + Word.size();

            if (
                !Current.empty() &&
                CandidateLength > MaximumCharacters
            )
            {
                Lines.push_back(Current);
                Current = Word;

                if (Lines.size() >= MaximumLines)
                    break;
            }
            else
            {
                if (!Current.empty())
                    Current += ' ';

                Current += Word;
            }
        }

        if (
            Lines.size() < MaximumLines &&
            !Current.empty()
        )
        {
            Lines.push_back(Current);
        }

        return Lines;
    }
}

void Renderer::DrawUpdateScreenV2(
    const UpdateVisualState& State
)
{
    glDisable(GL_DEPTH_TEST);

    const glm::vec3 Background{
        0.784f,
        0.733f,
        0.380f
    };

    const glm::vec3 WallpaperBand{
        0.748f,
        0.690f,
        0.330f
    };

    const glm::vec3 Ink{
        0.153f,
        0.137f,
        0.059f
    };

    const glm::vec3 Muted{
        0.255f,
        0.225f,
        0.083f
    };

    const glm::vec3 Track{
        0.640f,
        0.585f,
        0.275f
    };

    DrawRect(
        0,
        0,
        static_cast<int>(Width),
        static_cast<int>(Height),
        Background
    );

    for (
        int X = 24;
        X < static_cast<int>(Width);
        X += 24
    )
    {
        DrawRect(
            X,
            0,
            1,
            static_cast<int>(Height),
            WallpaperBand
        );
    }

    const int Margin =
        std::max(
            28,
            static_cast<int>(
                static_cast<float>(Width) * 0.035f
            )
        );

    DrawText(
        "BACKROOMS OFFICAL",
        Margin,
        28,
        2,
        Muted
    );

    const std::string VersionLine =
        State.RemoteVersion.empty()
            ? "CHECKING VERSION"
            : "CURRENT V" +
                State.CurrentVersion +
                "  LATEST V" +
                State.RemoteVersion;

    const int VersionWidth =
        TextWidth(VersionLine, 2);

    DrawText(
        VersionLine,
        static_cast<int>(Width) -
            Margin -
            VersionWidth,
        28,
        2,
        Muted
    );

    const int ContentX =
        std::max(
            40,
            static_cast<int>(
                static_cast<float>(Width) * 0.075f
            )
        );

    const int ContentY =
        std::max(
            130,
            static_cast<int>(
                static_cast<float>(Height) * 0.25f
            )
        );

    DrawText(
        "SYSTEM UPDATE",
        ContentX,
        ContentY,
        3,
        Muted
    );

    std::string MainTitle = "UPDATE NEEDED";

    if (State.Stage == UpdateStage::Checking)
        MainTitle = "CHECKING";
    else if (State.Stage == UpdateStage::ReadyToInstall)
        MainTitle = "UPDATE READY";
    else if (State.Stage == UpdateStage::Failed)
        MainTitle = "UPDATE FAILED";

    const int TitleScale =
        Width >= 1350 ? 10 :
        Width >= 1000 ? 8 : 6;

    DrawText(
        MainTitle,
        ContentX,
        ContentY + 42,
        TitleScale,
        Ink
    );

    std::string StatusText =
        SanitizeUpdateText(State.Message, 72);

    if (State.Stage == UpdateStage::Downloading)
    {
        StatusText =
            "DOWNLOADING BACKROOMS OFFICAL V" +
            State.RemoteVersion;
    }
    else if (State.Stage == UpdateStage::Verifying)
    {
        StatusText = "VERIFYING DOWNLOADED UPDATE";
    }
    else if (State.Stage == UpdateStage::ReadyToInstall)
    {
        StatusText = "READY TO REPLACE THE OLD BUILD IN PLACE";
    }

    DrawText(
        StatusText,
        ContentX,
        ContentY + 124,
        3,
        Ink
    );

    const int BarY = ContentY + 178;
    const int BarWidth =
        std::min(
            760,
            static_cast<int>(Width) -
                ContentX -
                Margin
        );

    DrawRect(
        ContentX,
        BarY,
        BarWidth,
        8,
        Track
    );

    float Progress =
        std::clamp(
            State.Progress,
            0.0f,
            1.0f
        );

    if (State.Stage == UpdateStage::ReadyToInstall)
        Progress = 1.0f;

    const int ProgressWidth =
        static_cast<int>(
            std::round(
                static_cast<float>(BarWidth) *
                Progress
            )
        );

    if (ProgressWidth > 0)
    {
        DrawRect(
            ContentX,
            BarY,
            ProgressWidth,
            8,
            Ink
        );
    }

    std::ostringstream PercentText;
    PercentText
        << static_cast<int>(
            std::round(Progress * 100.0f)
        )
        << "%";

    if (State.TotalBytes > 0)
    {
        PercentText
            << "   "
            << FormatMegabytes(State.DownloadedBytes)
            << " / "
            << FormatMegabytes(State.TotalBytes);
    }

    DrawText(
        PercentText.str(),
        ContentX,
        BarY + 22,
        2,
        Muted
    );

    DrawText(
        "GETTING",
        ContentX,
        BarY + 62,
        2,
        Muted
    );

    const std::string Package =
        State.PackageName.empty()
            ? "WAITING FOR PACKAGE INFORMATION"
            : SanitizeUpdateText(
                State.PackageName,
                72
            );

    DrawText(
        Package,
        ContentX,
        BarY + 84,
        2,
        Ink
    );

    DrawText(
        "INSTALL LOCATION",
        ContentX,
        BarY + 124,
        2,
        Muted
    );

    DrawText(
        "CURRENT GAME FOLDER  NO LOCATION PROMPT",
        ContentX,
        BarY + 146,
        2,
        Ink
    );

    const std::vector<std::string> Notes =
        WrapUpdateText(
            State.ReleaseNotes,
            68,
            2
        );

    if (!Notes.empty())
    {
        DrawText(
            "WHATS NEW",
            ContentX,
            BarY + 188,
            2,
            Muted
        );

        for (std::size_t I = 0; I < Notes.size(); ++I)
        {
            DrawText(
                Notes[I],
                ContentX,
                BarY + 210 +
                    static_cast<int>(I) * 20,
                2,
                Ink
            );
        }
    }

    std::string Action = "PLEASE WAIT";

    if (State.Stage == UpdateStage::UpdateAvailable)
        Action = "STARTING DOWNLOAD";
    else if (State.Stage == UpdateStage::Downloading)
        Action = "DOWNLOADING AUTOMATICALLY";
    else if (State.Stage == UpdateStage::Verifying)
        Action = "VERIFYING FILE";
    else if (State.Stage == UpdateStage::ReadyToInstall)
        Action = "ENTER  INSTALL UPDATE AND RESTART";
    else if (State.Stage == UpdateStage::Failed)
        Action = "ENTER  RETRY UPDATE";

    const int FooterY =
        static_cast<int>(Height) - 66;

    DrawRect(
        Margin,
        FooterY - 16,
        static_cast<int>(Width) - Margin * 2,
        1,
        Muted
    );

    DrawText(
        Action,
        Margin,
        FooterY,
        3,
        Ink
    );

    const std::string Safety =
        "DOWNLOAD IS VERIFIED BEFORE INSTALL";

    const int SafetyWidth =
        TextWidth(Safety, 2);

    DrawText(
        Safety,
        static_cast<int>(Width) -
            Margin -
            SafetyWidth,
        FooterY + 4,
        2,
        Muted
    );

    glEnable(GL_DEPTH_TEST);
}
