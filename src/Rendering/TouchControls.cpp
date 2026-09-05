#include "Renderer.h"

#include <algorithm>
#include <cmath>

void Renderer::DrawTouchControlsV1()
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    if (!GameplayTextRenderer.IsReady())
        return;

    GameplayTextRenderer.Resize(Width, Height);

    const glm::vec3 Ink{39.0f / 255.0f, 35.0f / 255.0f, 15.0f / 255.0f};
    const glm::vec3 Yellow{200.0f / 255.0f, 187.0f / 255.0f, 97.0f / 255.0f};
    const int ShortSide = std::max(1, static_cast<int>(std::min(Width, Height)));
    const int Pad = std::clamp(ShortSide / 45, 12, 24);
    const int StickSize = std::clamp(ShortSide / 6, 94, 150);
    const int ButtonWidth = std::clamp(ShortSide / 7, 82, 126);
    const int ButtonHeight = std::clamp(ShortSide / 14, 42, 68);

    auto Outline = [&](int X, int Y, int W, int H)
    {
        DrawRect(X, Y, W, 2, Ink);
        DrawRect(X, Y + H - 2, W, 2, Ink);
        DrawRect(X, Y, 2, H, Ink);
        DrawRect(X + W - 2, Y, 2, H, Ink);
    };

    const int MoveX = Pad;
    const int MoveY = static_cast<int>(Height) - Pad - StickSize;
    Outline(MoveX, MoveY, StickSize, StickSize);
    DrawRect(
        MoveX + StickSize / 2 - 1,
        MoveY + 12,
        2,
        StickSize - 24,
        Ink
    );
    DrawRect(
        MoveX + 12,
        MoveY + StickSize / 2 - 1,
        StickSize - 24,
        2,
        Ink
    );
    GameplayTextRenderer.Draw(
        "MOVE",
        MoveX + 10,
        MoveY + 8,
        9,
        800,
        0.08f,
        Ink,
        1.0f,
        false
    );

    const int MenuX = static_cast<int>(Width) - Pad - ButtonWidth;
    const int MenuY = Pad;
    Outline(MenuX, MenuY, ButtonWidth, ButtonHeight);
    GameplayTextRenderer.Draw(
        "MENU",
        MenuX + 12,
        MenuY + (ButtonHeight - 11) / 2,
        11,
        850,
        0.08f,
        Ink,
        1.0f,
        false
    );

    const int MapX = MenuX - Pad / 2 - ButtonWidth;
    Outline(MapX, MenuY, ButtonWidth, ButtonHeight);
    GameplayTextRenderer.Draw(
        "MAP",
        MapX + 12,
        MenuY + (ButtonHeight - 11) / 2,
        11,
        850,
        0.08f,
        Ink,
        1.0f,
        false
    );

    const int UseY = static_cast<int>(Height) - Pad - ButtonHeight;
    Outline(MenuX, UseY, ButtonWidth, ButtonHeight);
    GameplayTextRenderer.Draw(
        "USE",
        MenuX + 12,
        UseY + (ButtonHeight - 11) / 2,
        11,
        850,
        0.08f,
        Ink,
        1.0f,
        false
    );

    const int LookWidth = std::clamp(ShortSide / 5, 120, 190);
    const int LookHeight = std::clamp(ShortSide / 9, 62, 96);
    const int LookX = static_cast<int>(Width) - Pad - LookWidth;
    const int LookY = static_cast<int>(Height) / 2 - LookHeight / 2;
    DrawRect(LookX, LookY, LookWidth, 1, Ink);
    DrawRect(LookX, LookY + LookHeight - 1, LookWidth, 1, Ink);
    GameplayTextRenderer.Draw(
        "DRAG TO LOOK",
        LookX + 10,
        LookY + LookHeight / 2 - 5,
        9,
        700,
        0.05f,
        Ink,
        0.85f,
        false
    );

    static_cast<void>(Yellow);
}
