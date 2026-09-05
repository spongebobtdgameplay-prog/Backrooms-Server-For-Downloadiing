#include "Renderer.h"

#include "../Core/Version.h"
#include "MenuLayout.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    glm::vec3 Hex(unsigned int Value)
    {
        return {
            static_cast<float>((Value >> 16) & 0xffu) / 255.0f,
            static_cast<float>((Value >> 8) & 0xffu) / 255.0f,
            static_cast<float>(Value & 0xffu) / 255.0f
        };
    }

    glm::vec3 Mix(
        const glm::vec3& A,
        const glm::vec3& B,
        float T
    )
    {
        T = std::clamp(T, 0.0f, 1.0f);
        return A * (1.0f - T) + B * T;
    }

    void CenterText(
        Renderer& RendererRef,
        const std::string& Text,
        int CenterX,
        int Y,
        int PixelHeight,
        int Weight,
        float Tracking,
        const glm::vec3& Color,
        float Opacity = 1.0f
    )
    {
        const int Width = RendererRef.MenuTextWidth(
            Text,
            PixelHeight,
            Weight,
            Tracking
        );

        RendererRef.DrawMenuText(
            Text,
            CenterX - Width / 2,
            Y,
            PixelHeight,
            Weight,
            Tracking,
            Color,
            Opacity,
            false
        );
    }
}

void Renderer::DrawClassicBackdrop()
{
    const glm::vec3 Base = Hex(0x090908);
    const glm::vec3 Warm = Hex(0x8d7e36);

    DrawRect(
        0,
        0,
        static_cast<int>(Width),
        static_cast<int>(Height),
        Base
    );

    const int CenterX = static_cast<int>(Width) / 2;
    const int CenterY = static_cast<int>(Height) / 2;
    const int MaxRadius = std::max(
        1,
        std::min(
            static_cast<int>(Width),
            static_cast<int>(Height)
        ) / 2
    );

    // Approximate the original browser radial-gradient without inventing
    // another visual theme. The center only gets the same subtle warm lift.
    for (int Step = 12; Step >= 1; --Step)
    {
        const float T = static_cast<float>(Step) / 12.0f;
        const int Radius = static_cast<int>(MaxRadius * T);
        const float Strength = 0.08f * (1.0f - T + 0.08f);
        const glm::vec3 Color = Mix(Base, Warm, Strength);

        DrawRect(
            CenterX - Radius,
            CenterY - Radius,
            Radius * 2,
            Radius * 2,
            Color
        );
    }
}

void Renderer::DrawClassicButton(
    const MenuLayout::Rect& Rect,
    const std::string& Label,
    bool Hovered
)
{
    const glm::vec3 Border = Hex(0xd6cb7b);
    const glm::vec3 Fill = Hovered
        ? Hex(0xe0d681)
        : Hex(0xcfc476);
    const glm::vec3 Text = Hex(0x17150d);

    DrawRect(
        Rect.X,
        Rect.Y,
        Rect.Width,
        Rect.Height,
        Border
    );

    DrawRect(
        Rect.X + 1,
        Rect.Y + 1,
        std::max(0, Rect.Width - 2),
        std::max(0, Rect.Height - 2),
        Fill
    );

    const int FontSize = 13;
    const float Tracking = 0.23f;
    const int TextWidth = MenuTextWidth(
        Label,
        FontSize,
        800,
        Tracking
    );

    DrawMenuText(
        Label,
        Rect.X + Rect.Width / 2 - TextWidth / 2,
        Rect.Y + (Rect.Height - FontSize) / 2,
        FontSize,
        800,
        Tracking,
        Text,
        1.0f,
        false
    );
}

void Renderer::DrawClassicMainMenu(bool HasSession)
{
    glDisable(GL_DEPTH_TEST);
    DrawClassicBackdrop();

    const auto Layout = MenuLayout::BuildMainMenu(
        Width,
        Height,
        HasSession
    );

    const glm::vec3 PanelFill = Hex(0x0f0e09);
    const glm::vec3 PanelBorder = Hex(0x4a472f);
    const glm::vec3 Shadow = Hex(0x030302);
    const glm::vec3 Title = Hex(0xf4efbd);
    const glm::vec3 Paragraph = Hex(0xc9c293);
    const glm::vec3 Eyebrow = Hex(0x8b845d);
    const glm::vec3 Muted = Hex(0x77715a);

    // Original .Panel: centered, bordered, dark translucent card with a heavy shadow.
    DrawRect(
        Layout.Panel.X + 14,
        Layout.Panel.Y + 18,
        Layout.Panel.Width,
        Layout.Panel.Height,
        Shadow
    );

    DrawRect(
        Layout.Panel.X,
        Layout.Panel.Y,
        Layout.Panel.Width,
        Layout.Panel.Height,
        PanelBorder
    );

    DrawRect(
        Layout.Panel.X + 1,
        Layout.Panel.Y + 1,
        Layout.Panel.Width - 2,
        Layout.Panel.Height - 2,
        PanelFill
    );

    const int CenterX = Layout.Panel.X + Layout.Panel.Width / 2;
    const int Top = Layout.Panel.Y + 38;

    CenterText(
        *this,
        "LEVEL 0",
        CenterX,
        Top,
        12,
        600,
        0.58f,
        Eyebrow,
        1.0f
    );

    const int TitleSize = std::clamp(
        static_cast<int>(std::round(static_cast<float>(Width) * 0.08f)),
        40,
        76
    );

    CenterText(
        *this,
        "THE LOBBY",
        CenterX,
        Top + 34,
        TitleSize,
        900,
        -4.0f / static_cast<float>(TitleSize),
        Title,
        1.0f
    );

    const int ParagraphY = Top + 34 + TitleSize + 28;

    CenterText(
        *this,
        "Mono-yellow rooms, damp carpet and fluorescent light.",
        CenterX,
        ParagraphY,
        15,
        400,
        0.0f,
        Paragraph,
        1.0f
    );

    CenterText(
        *this,
        "Restore three breakers and find the powered exit.",
        CenterX,
        ParagraphY + 23,
        15,
        400,
        0.0f,
        Paragraph,
        1.0f
    );

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    SDL_GetMouseState(&MouseX, &MouseY);

    DrawClassicButton(
        Layout.PrimaryButton,
        HasSession ? "RESUME SESSION" : "ENTER LEVEL 0",
        Layout.PrimaryButton.Contains(MouseX, MouseY)
    );

    if (Layout.HasSecondaryButton)
    {
        DrawClassicButton(
            Layout.SecondaryButton,
            "NEW SESSION",
            Layout.SecondaryButton.Contains(MouseX, MouseY)
        );
    }

    CenterText(
        *this,
        std::string("BACKROOMS OFFICAL  V") + BuildVersion::Text,
        CenterX,
        Layout.Panel.Y + Layout.Panel.Height - 20,
        10,
        400,
        0.10f,
        Muted,
        1.0f
    );

    glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawClassicPauseMenu()
{
    glDisable(GL_DEPTH_TEST);
    DrawClassicBackdrop();

    const auto Layout = MenuLayout::BuildPauseMenu(
        Width,
        Height
    );

    const glm::vec3 PanelFill = Hex(0x0f0e09);
    const glm::vec3 PanelBorder = Hex(0x4a472f);
    const glm::vec3 Shadow = Hex(0x030302);
    const glm::vec3 Title = Hex(0xf4efbd);
    const glm::vec3 Eyebrow = Hex(0x8b845d);

    DrawRect(
        Layout.Panel.X + 14,
        Layout.Panel.Y + 18,
        Layout.Panel.Width,
        Layout.Panel.Height,
        Shadow
    );

    DrawRect(
        Layout.Panel.X,
        Layout.Panel.Y,
        Layout.Panel.Width,
        Layout.Panel.Height,
        PanelBorder
    );

    DrawRect(
        Layout.Panel.X + 1,
        Layout.Panel.Y + 1,
        Layout.Panel.Width - 2,
        Layout.Panel.Height - 2,
        PanelFill
    );

    const int CenterX = Layout.Panel.X + Layout.Panel.Width / 2;

    CenterText(
        *this,
        "LEVEL 0",
        CenterX,
        Layout.Panel.Y + 34,
        12,
        600,
        0.58f,
        Eyebrow,
        1.0f
    );

    CenterText(
        *this,
        "SESSION PAUSED",
        CenterX,
        Layout.Panel.Y + 76,
        46,
        900,
        -0.04f,
        Title,
        1.0f
    );

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    SDL_GetMouseState(&MouseX, &MouseY);

    DrawClassicButton(
        Layout.ResumeButton,
        "RESUME",
        Layout.ResumeButton.Contains(MouseX, MouseY)
    );

    DrawClassicButton(
        Layout.MainMenuButton,
        "MAIN MENU",
        Layout.MainMenuButton.Contains(MouseX, MouseY)
    );

    glEnable(GL_DEPTH_TEST);
}
