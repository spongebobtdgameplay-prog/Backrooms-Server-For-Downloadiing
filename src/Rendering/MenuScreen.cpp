#include "Renderer.h"

#include "../Core/Version.h"

#include <algorithm>
#include <cmath>
#include <string>

void Renderer::DrawMenuBackdrop()
{
    const glm::vec3 Background{
        0.784f,
        0.733f,
        0.380f
    };

    const glm::vec3 VerticalBand{
        0.758f,
        0.701f,
        0.342f
    };

    const glm::vec3 HorizontalBand{
        0.796f,
        0.746f,
        0.397f
    };

    DrawRect(
        0,
        0,
        static_cast<int>(Width),
        static_cast<int>(Height),
        Background
    );

    for (
        int X = 23;
        X < static_cast<int>(Width);
        X += 24
    )
    {
        DrawRect(
            X,
            0,
            2,
            static_cast<int>(Height),
            VerticalBand
        );
    }

    for (
        int Y = 31;
        Y < static_cast<int>(Height);
        Y += 32
    )
    {
        DrawRect(
            0,
            Y,
            static_cast<int>(Width),
            1,
            HorizontalBand
        );
    }

    // Approximate the original CSS vignette without turning the menu
    // into the flat yellow card that the first native port used.
    const int VignetteSteps = 18;

    for (int I = 0; I < VignetteSteps; ++I)
    {
        const float T =
            static_cast<float>(I) /
            static_cast<float>(VignetteSteps - 1);

        const float Strength = 0.035f * (1.0f - T);

        const glm::vec3 Shade =
            Background * (1.0f - Strength);

        const int Thickness = 2;
        const int Offset = I * Thickness;

        DrawRect(
            0,
            Offset,
            static_cast<int>(Width),
            Thickness,
            Shade
        );

        DrawRect(
            0,
            static_cast<int>(Height) - Offset - Thickness,
            static_cast<int>(Width),
            Thickness,
            Shade
        );

        DrawRect(
            Offset,
            0,
            Thickness,
            static_cast<int>(Height),
            Shade
        );

        DrawRect(
            static_cast<int>(Width) - Offset - Thickness,
            0,
            Thickness,
            static_cast<int>(Height),
            Shade
        );
    }
}

void Renderer::DrawMenuText(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color,
    float Opacity,
    bool Shadow
)
{
    if (!MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Initialize();
        MenuTextRenderer.Resize(Width, Height);
    }

    if (MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Resize(Width, Height);
        MenuTextRenderer.Draw(
            Text,
            X,
            Y,
            PixelHeight,
            Weight,
            TrackingEm,
            Color,
            Opacity,
            Shadow
        );
        return;
    }

    const int FallbackScale =
        std::max(1, PixelHeight / 8);

    DrawText(
        Text,
        X,
        Y,
        FallbackScale,
        Color
    );
}

int Renderer::MenuTextWidth(
    const std::string& Text,
    int PixelHeight,
    int Weight,
    float TrackingEm
)
{
    if (!MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Initialize();
        MenuTextRenderer.Resize(Width, Height);
    }

    if (MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Resize(Width, Height);
        return MenuTextRenderer.Measure(
            Text,
            PixelHeight,
            Weight,
            TrackingEm
        );
    }

    const int FallbackScale =
        std::max(1, PixelHeight / 8);

    return TextWidth(
        Text,
        FallbackScale
    );
}

void Renderer::DrawMainMenuV2(bool HasSession)
{
    glDisable(GL_DEPTH_TEST);

    DrawMenuBackdrop();

    const glm::vec3 Ink{
        0.153f,
        0.137f,
        0.059f
    };

    const glm::vec3 Muted{
        0.270f,
        0.235f,
        0.095f
    };

    const glm::vec3 Faint{
        0.355f,
        0.313f,
        0.137f
    };

    const int Margin = 34;

    DrawMenuText(
        "THE BACKROOMS",
        Margin,
        28,
        10,
        400,
        0.20f,
        Faint
    );

    const std::string Version =
        std::string("V") +
        BuildVersion::Text;

    const int VersionWidth =
        MenuTextWidth(
            Version,
            10,
            400,
            0.20f
        );

    DrawMenuText(
        Version,
        static_cast<int>(Width) / 2 -
            VersionWidth / 2,
        28,
        10,
        400,
        0.20f,
        Faint
    );

    const std::string Status =
        HasSession
            ? "SESSION ACTIVE"
            : "UNSTABLE SESSION";

    const int StatusWidth =
        MenuTextWidth(
            Status,
            10,
            400,
            0.20f
        );

    DrawMenuText(
        Status,
        std::max(
            Margin,
            static_cast<int>(Width) -
                Margin -
                StatusWidth
        ),
        28,
        10,
        400,
        0.20f,
        Faint
    );

    const int ContentX =
        std::clamp(
            static_cast<int>(
                static_cast<float>(Width) *
                0.07f
            ),
            34,
            110
        );

    const int ContentBottom =
        std::clamp(
            static_cast<int>(
                static_cast<float>(Height) *
                0.13f
            ),
            74,
            150
        );

    const int ContentY =
        std::max(
            105,
            static_cast<int>(Height) -
                ContentBottom -
                330
        );

    DrawMenuText(
        "LEVEL 0",
        ContentX,
        ContentY,
        12,
        400,
        0.35f,
        Muted
    );

    DrawRect(
        ContentX,
        ContentY + 27,
        104,
        1,
        Muted
    );

    const int TitleSize =
        std::clamp(
            static_cast<int>(
                static_cast<float>(Width) *
                0.10f
            ),
            66,
            138
        );

    DrawMenuText(
        "THE LOBBY",
        ContentX,
        ContentY + 40,
        TitleSize,
        900,
        -0.07f,
        Ink
    );

    const int BodyY =
        ContentY +
        std::max(128, TitleSize - 2);

    DrawMenuText(
        "Mono-yellow rooms, damp carpet and fluorescent light with no reliable layout.",
        ContentX,
        BodyY,
        15,
        600,
        0.0f,
        Muted
    );

    DrawMenuText(
        "Restore three breakers and find the powered exit.",
        ContentX,
        BodyY + 28,
        15,
        600,
        0.0f,
        Muted
    );

    const int ButtonY = BodyY + 78;
    const int ButtonWidth = 360;

    DrawRect(
        ContentX,
        ButtonY,
        ButtonWidth,
        1,
        Muted
    );

    const std::string PrimaryAction =
        HasSession
            ? "RESUME SESSION"
            : "ENTER LEVEL 0";

    DrawMenuText(
        PrimaryAction,
        ContentX + 4,
        ButtonY + 16,
        12,
        800,
        0.17f,
        Ink
    );

    DrawMenuText(
        ">",
        ContentX + ButtonWidth - 24,
        ButtonY + 12,
        20,
        700,
        0.0f,
        Ink
    );

    DrawRect(
        ContentX,
        ButtonY + 58,
        ButtonWidth,
        1,
        Muted
    );

    DrawMenuText(
        HasSession
            ? "N  NEW SESSION"
            : "WASD  SHIFT  MOUSE  E",
        ContentX,
        ButtonY + 76,
        10,
        400,
        0.16f,
        Faint
    );

    DrawMenuText(
        "NOCLIP DESTINATION",
        Margin,
        static_cast<int>(Height) - 31,
        10,
        400,
        0.20f,
        Faint
    );

    const std::string Footer =
        HasSession
            ? "ESC PAUSE   M MAIN MENU"
            : "ENTER STARTS LEVEL 0";

    const int FooterWidth =
        MenuTextWidth(
            Footer,
            10,
            400,
            0.20f
        );

    DrawMenuText(
        Footer,
        std::max(
            Margin,
            static_cast<int>(Width) -
                Margin -
                FooterWidth
        ),
        static_cast<int>(Height) - 31,
        10,
        400,
        0.20f,
        Faint
    );

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawPauseMenuV2()
{
    glDisable(GL_DEPTH_TEST);

    DrawMenuBackdrop();

    const glm::vec3 Ink{
        0.153f,
        0.137f,
        0.059f
    };

    const glm::vec3 Muted{
        0.270f,
        0.235f,
        0.095f
    };

    const int ContentX =
        std::clamp(
            static_cast<int>(
                static_cast<float>(Width) *
                0.07f
            ),
            34,
            110
        );

    const int ContentY =
        std::max(
            150,
            static_cast<int>(
                static_cast<float>(Height) *
                0.42f
            )
        );

    DrawMenuText(
        "LEVEL 0",
        ContentX,
        ContentY,
        12,
        400,
        0.35f,
        Muted
    );

    const int TitleSize =
        std::clamp(
            static_cast<int>(
                static_cast<float>(Width) *
                0.075f
            ),
            58,
            108
        );

    DrawMenuText(
        "SESSION PAUSED",
        ContentX,
        ContentY + 36,
        TitleSize,
        900,
        -0.055f,
        Ink
    );

    const int ButtonY =
        ContentY + TitleSize + 74;

    DrawRect(
        ContentX,
        ButtonY,
        410,
        1,
        Muted
    );

    DrawMenuText(
        "ENTER  RESUME",
        ContentX + 4,
        ButtonY + 16,
        18,
        800,
        0.13f,
        Ink
    );

    DrawRect(
        ContentX,
        ButtonY + 58,
        410,
        1,
        Muted
    );

    DrawMenuText(
        "M  MAIN MENU",
        ContentX + 4,
        ButtonY + 78,
        18,
        800,
        0.13f,
        Ink
    );

    DrawMenuText(
        "ESC ALSO RESUMES",
        ContentX,
        ButtonY + 126,
        13,
        500,
        0.14f,
        Muted
    );

    glEnable(GL_DEPTH_TEST);
}
