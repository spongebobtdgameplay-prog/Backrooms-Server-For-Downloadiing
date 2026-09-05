#include "Renderer.h"

#include "../Core/Version.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
    glm::vec3 LevelZeroYellow()
    {
        return {200.0f / 255.0f, 187.0f / 255.0f, 97.0f / 255.0f};
    }

    glm::vec3 LevelZeroInk()
    {
        return {39.0f / 255.0f, 35.0f / 255.0f, 15.0f / 255.0f};
    }
}

void Renderer::DrawMapPanel(
    const WorldData& World,
    int X,
    int Y,
    int PanelWidth,
    int PanelHeight,
    const glm::vec2& MapCenter,
    float PixelsPerMeter,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    bool Detailed
)
{
    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 PatternA{0.735f, 0.684f, 0.335f};
    const glm::vec3 PatternB{0.805f, 0.755f, 0.405f};
    const glm::vec3 Wall{0.16f, 0.145f, 0.075f};

    auto ClipRect = [&](
        int RectX,
        int RectY,
        int RectWidth,
        int RectHeight,
        const glm::vec3& Color
    )
    {
        const int Left = std::max(RectX, X);
        const int Top = std::max(RectY, Y);
        const int Right = std::min(RectX + RectWidth, X + PanelWidth);
        const int Bottom = std::min(RectY + RectHeight, Y + PanelHeight);

        if (Right > Left && Bottom > Top)
            DrawRect(Left, Top, Right - Left, Bottom - Top, Color);
    };

    DrawRect(X, Y, PanelWidth, PanelHeight, Yellow);

    for (int StripeX = X; StripeX < X + PanelWidth; StripeX += 18)
        ClipRect(StripeX, Y, 2, PanelHeight, PatternA);

    for (int StripeX = X + 7; StripeX < X + PanelWidth; StripeX += 36)
        ClipRect(StripeX, Y, 1, PanelHeight, PatternB);

    for (int StripeY = Y + 5; StripeY < Y + PanelHeight; StripeY += 14)
        ClipRect(X, StripeY, PanelWidth, 1, PatternB);

    const float Scale = std::max(PixelsPerMeter, 0.1f);
    const float HalfCell = World.CellSize * 0.5f;

    auto ToScreen = [&](float WorldX, float WorldZ)
    {
        return glm::ivec2{
            X + PanelWidth / 2 +
                static_cast<int>(std::round((WorldX - MapCenter.x) * Scale)),
            Y + PanelHeight / 2 +
                static_cast<int>(std::round((WorldZ - MapCenter.y) * Scale))
        };
    };

    const int WallThickness = Detailed ? 2 : 1;

    for (int LocalZ = 0; LocalZ < World.Rows; ++LocalZ)
    {
        for (int LocalX = 0; LocalX < World.Columns; ++LocalX)
        {
            const MazeCell& Cell = World.Cell(LocalX, LocalZ);
            const int WorldCellX = World.OriginCellX + LocalX;
            const int WorldCellZ = World.OriginCellZ + LocalZ;
            const float CenterX = static_cast<float>(WorldCellX) * World.CellSize;
            const float CenterZ = static_cast<float>(WorldCellZ) * World.CellSize;

            const glm::ivec2 TopLeft =
                ToScreen(CenterX - HalfCell, CenterZ - HalfCell);
            const glm::ivec2 BottomRight =
                ToScreen(CenterX + HalfCell, CenterZ + HalfCell);

            const int CellWidth = std::max(BottomRight.x - TopLeft.x, 1);
            const int CellHeight = std::max(BottomRight.y - TopLeft.y, 1);

            if (Detailed)
            {
                const int TintIndex =
                    (std::abs(WorldCellX * 31 + WorldCellZ * 17) % 3);
                const glm::vec3 RoomTint =
                    TintIndex == 0
                        ? glm::vec3{0.79f, 0.735f, 0.365f}
                        : TintIndex == 1
                            ? glm::vec3{0.765f, 0.712f, 0.345f}
                            : glm::vec3{0.81f, 0.755f, 0.39f};

                ClipRect(
                    TopLeft.x + 2,
                    TopLeft.y + 2,
                    std::max(CellWidth - 4, 1),
                    std::max(CellHeight - 4, 1),
                    RoomTint
                );
            }

            if (Cell.Walls[0])
                ClipRect(TopLeft.x, TopLeft.y, CellWidth, WallThickness, Wall);

            if (Cell.Walls[1])
                ClipRect(BottomRight.x - WallThickness, TopLeft.y, WallThickness, CellHeight, Wall);

            if (Cell.Walls[2])
                ClipRect(TopLeft.x, BottomRight.y - WallThickness, CellWidth, WallThickness, Wall);

            if (Cell.Walls[3])
                ClipRect(TopLeft.x, TopLeft.y, WallThickness, CellHeight, Wall);
        }
    }

    auto DrawMarker = [&](const MapMarker& Marker)
    {
        const glm::ivec2 Position = ToScreen(Marker.Position.x, Marker.Position.y);

        if (
            Position.x < X - 10 ||
            Position.y < Y - 10 ||
            Position.x > X + PanelWidth + 10 ||
            Position.y > Y + PanelHeight + 10
        )
        {
            return;
        }

        glm::vec3 Color{0.95f, 0.62f, 0.12f};
        int Radius = Detailed ? 5 : 4;

        if (Marker.Kind == MapMarkerKind::BreakerActive)
            Color = {0.12f, 0.82f, 0.24f};
        else if (Marker.Kind == MapMarkerKind::Exit)
            Color = {0.88f, 0.88f, 0.72f};
        else if (Marker.Kind == MapMarkerKind::ExitPowered)
            Color = {0.20f, 0.92f, 0.48f};
        else if (Marker.Kind == MapMarkerKind::Entity)
        {
            Color = {0.92f, 0.10f, 0.06f};
            Radius = Detailed ? 6 : 5;
        }

        ClipRect(Position.x - Radius - 1, Position.y - Radius - 1, Radius * 2 + 2, Radius * 2 + 2, Ink);
        ClipRect(Position.x - Radius, Position.y - Radius, Radius * 2, Radius * 2, Color);
        ClipRect(Position.x - 1, Position.y - Radius - 3, 2, 3, Color);
    };

    for (const MapMarker& Marker : Markers)
        DrawMarker(Marker);

    const glm::ivec2 PlayerScreen =
        ToScreen(PlayerPosition.x, PlayerPosition.y);

    glm::vec2 Direction = PlayerForward;

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const glm::vec3 PlayerColor{0.98f, 0.97f, 0.84f};

    ClipRect(PlayerScreen.x - 4, PlayerScreen.y - 4, 8, 8, Ink);
    ClipRect(PlayerScreen.x - 3, PlayerScreen.y - 3, 6, 6, PlayerColor);

    for (int Step = 1; Step <= 8; ++Step)
    {
        const int DotX =
            PlayerScreen.x +
            static_cast<int>(std::round(Direction.x * static_cast<float>(Step) * 1.5f));
        const int DotY =
            PlayerScreen.y +
            static_cast<int>(std::round(Direction.y * static_cast<float>(Step) * 1.5f));

        ClipRect(DotX - 1, DotY - 1, 3, 3, PlayerColor);
    }

    DrawRect(X, Y, PanelWidth, 2, Ink);
    DrawRect(X, Y + PanelHeight - 2, PanelWidth, 2, Ink);
    DrawRect(X, Y, 2, PanelHeight, Ink);
    DrawRect(X + PanelWidth - 2, Y, 2, PanelHeight, Ink);
}

void Renderer::DrawMiniMapV1(
    const WorldData& World,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers
)
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    const int MapWidth =
        std::clamp(static_cast<int>(Width) / 6, 220, 270);
    const int MapHeight =
        std::clamp(static_cast<int>(Height) / 5, 138, 170);
    const int MapX = Width <= 700 ? 18 : 26;
    const int MapY =
        static_cast<int>(Height) - MapHeight - 28;

    DrawMapPanel(
        World,
        MapX,
        MapY,
        MapWidth,
        MapHeight,
        PlayerPosition,
        1.72f,
        PlayerPosition,
        PlayerForward,
        Markers,
        false
    );

    if (GameplayTextRenderer.IsReady())
    {
        GameplayTextRenderer.Resize(Width, Height);
        const glm::vec3 Ink = LevelZeroInk();

        GameplayTextRenderer.Draw(
            "M  MAP",
            MapX + 10,
            MapY + MapHeight - 20,
            9,
            800,
            0.12f,
            Ink,
            1.0f,
            false
        );
    }
}

void Renderer::DrawFullMapV1(
    const WorldData& World,
    const glm::vec2& MapCenter,
    float Zoom,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    int BreakersActive,
    int BreakersRequired
)
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    GameplayTextRenderer.Resize(Width, Height);

    glDisable(GL_DEPTH_TEST);

    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 Muted{0.33f, 0.30f, 0.13f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Yellow);

    for (int X = 0; X < static_cast<int>(Width); X += 22)
        DrawRect(X, 0, 2, static_cast<int>(Height), {0.735f, 0.684f, 0.335f});

    for (int Y = 7; Y < static_cast<int>(Height); Y += 16)
        DrawRect(0, Y, static_cast<int>(Width), 1, {0.805f, 0.755f, 0.405f});

    const int Margin = Width <= 700 ? 18 : 34;
    const int HeaderY = 20;

    GameplayTextRenderer.Draw(
        "MAP",
        Margin,
        HeaderY,
        30,
        900,
        0.05f,
        Ink,
        1.0f,
        false
    );

    GameplayTextRenderer.Draw(
        "LEVEL 0   /   INFINITE GRID",
        Margin + 96,
        HeaderY + 8,
        11,
        750,
        0.14f,
        Muted,
        1.0f,
        false
    );

    std::ostringstream Objective;
    Objective << "RESTORE POWER  " << BreakersActive << "/" << BreakersRequired;

    const std::string ObjectiveText = Objective.str();
    const int ObjectiveWidth =
        GameplayTextRenderer.Measure(ObjectiveText, 11, 800, 0.12f);

    GameplayTextRenderer.Draw(
        ObjectiveText,
        static_cast<int>(Width) - Margin - ObjectiveWidth,
        HeaderY + 8,
        11,
        800,
        0.12f,
        Ink,
        1.0f,
        false
    );

    DrawRect(Margin, 70, static_cast<int>(Width) - Margin * 2, 2, Ink);

    const int MapX = Margin;
    const int MapY = 92;
    const int MapWidth = std::max(static_cast<int>(Width) - Margin * 2, 1);
    const int MapHeight = std::max(static_cast<int>(Height) - MapY - 62, 1);

    DrawMapPanel(
        World,
        MapX,
        MapY,
        MapWidth,
        MapHeight,
        MapCenter,
        8.0f * std::clamp(Zoom, 0.65f, 2.8f),
        PlayerPosition,
        PlayerForward,
        Markers,
        true
    );

    std::ostringstream ZoomText;
    ZoomText << "ZOOM " << std::fixed << std::setprecision(0) << std::clamp(Zoom, 0.65f, 2.8f) * 100.0f << "%";

    GameplayTextRenderer.Draw(
        "WASD / ARROWS PAN    DRAG PAN    WHEEL ZOOM    R RECENTER    M OR ESC BACK",
        Margin,
        static_cast<int>(Height) - 34,
        9,
        700,
        0.10f,
        Muted,
        1.0f,
        false
    );

    const int ZoomWidth =
        GameplayTextRenderer.Measure(ZoomText.str(), 9, 750, 0.10f);

    GameplayTextRenderer.Draw(
        ZoomText.str(),
        static_cast<int>(Width) - Margin - ZoomWidth,
        static_cast<int>(Height) - 34,
        9,
        750,
        0.10f,
        Ink,
        1.0f,
        false
    );

    glEnable(GL_DEPTH_TEST);
}
