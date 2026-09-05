from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, content):
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="utf-8")


def replace_once(text, old, new, label):
    if old not in text:
        raise RuntimeError(f"Missing anchor: {label}")
    return text.replace(old, new, 1)


world_h = read("src/World/WorldGenerator.h")
world_h = replace_once(
    world_h,
    "    WorldData BuildMapAround(const glm::vec3& FocusPosition);\n",
    "    WorldData BuildMapAround(const glm::vec3& FocusPosition);\n    WorldData BuildMapRegion(\n        const glm::vec3& FocusPosition,\n        int ChunkRadius\n    );\n",
    "WorldGenerator map declaration",
)
write("src/World/WorldGenerator.h", world_h)

world_cpp = read("src/World/WorldGenerator.cpp")
pattern = re.compile(
    r"WorldData WorldGenerator::BuildMapAround\(\n    const glm::vec3& FocusPosition\n\)\n\{.*?\n\}\n\nbool WorldGenerator::NeedsRebuild",
    re.S,
)
replacement = '''WorldData WorldGenerator::BuildMapAround(
    const glm::vec3& FocusPosition
)
{
    return BuildMapRegion(FocusPosition, ActiveChunkRadius);
}

WorldData WorldGenerator::BuildMapRegion(
    const glm::vec3& FocusPosition,
    int ChunkRadius
)
{
    WorldData World;

    const int Radius = std::clamp(ChunkRadius, 1, 40);

    World.ChunkCells = ChunkCellCount;
    World.StreamRadius = Radius;
    World.Columns = ChunkCellCount * (Radius * 2 + 1);
    World.Rows = World.Columns;
    World.CellSize = DefaultCellSize;

    World.CenterChunkX = ChunkCoordinate(FocusPosition.x);
    World.CenterChunkZ = ChunkCoordinate(FocusPosition.z);

    const int FirstChunkX = World.CenterChunkX - Radius;
    const int FirstChunkZ = World.CenterChunkZ - Radius;

    World.OriginCellX =
        FirstChunkX * ChunkCellCount -
        ChunkHalfCells;

    World.OriginCellZ =
        FirstChunkZ * ChunkCellCount -
        ChunkHalfCells;

    CreateStreamedMaze(World);
    return World;
}

bool WorldGenerator::NeedsRebuild'''
world_cpp, count = pattern.subn(replacement, world_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace BuildMapAround")
write("src/World/WorldGenerator.cpp", world_cpp)

renderer_h = read("src/Rendering/Renderer.h")
renderer_h = replace_once(
    renderer_h,
    '''struct MapMarker
{
    glm::vec2 Position{0.0f};
    MapMarkerKind Kind = MapMarkerKind::Breaker;
};
''',
    '''struct MapMarker
{
    glm::vec2 Position{0.0f};
    MapMarkerKind Kind = MapMarkerKind::Breaker;
};

enum class MapUiAction
{
    None,
    Back,
    RandomDestination,
    ClearWaypoint
};

struct MapWaypointView
{
    bool Active = false;
    glm::vec2 Position{0.0f};
    float DistanceMeters = 0.0f;
};
''',
    "Map UI structs",
)
old_map_public = '''    void DrawMiniMapV1(
        const WorldData& World,
        const glm::vec2& PlayerPosition,
        const glm::vec2& PlayerForward,
        const std::vector<MapMarker>& Markers
    );

    void DrawFullMapV1(
        const WorldData& World,
        const glm::vec2& MapCenter,
        float Zoom,
        const glm::vec2& PlayerPosition,
        const glm::vec2& PlayerForward,
        const std::vector<MapMarker>& Markers,
        int BreakersActive,
        int BreakersRequired
    );
'''
new_map_public = '''    void DrawMiniMapV2(
        const WorldData& World,
        const glm::vec2& PlayerPosition,
        const glm::vec2& PlayerForward,
        const std::vector<MapMarker>& Markers,
        const MapWaypointView& Waypoint,
        const std::vector<glm::vec2>& Route,
        float ThreatDistance,
        float Time
    );

    void DrawFullMapV2(
        const WorldData& World,
        const glm::vec2& MapCenter,
        float Zoom,
        const glm::vec2& PlayerPosition,
        const glm::vec2& PlayerForward,
        const std::vector<MapMarker>& Markers,
        const MapWaypointView& Waypoint,
        const std::vector<glm::vec2>& Route,
        int BreakersActive,
        int BreakersRequired,
        float Time
    );

    void DrawTouchControlsV1();
    MapUiAction HitTestFullMap() const;
    bool PointerInsideFullMap() const;
    glm::vec2 FullMapPointerToWorld(
        const glm::vec2& MapCenter,
        float Zoom
    ) const;
'''
renderer_h = replace_once(renderer_h, old_map_public, new_map_public, "Renderer map public API")
old_panel = '''    void DrawMapPanel(
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
    );
'''
new_panel = '''    void DrawMapPanelV2(
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
        const MapWaypointView& Waypoint,
        const std::vector<glm::vec2>& Route,
        bool Detailed,
        float Time
    );
'''
renderer_h = replace_once(renderer_h, old_panel, new_panel, "Renderer map panel API")
renderer_h = replace_once(
    renderer_h,
    '''    UiRect PauseMapRect;
    UiRect PauseResumeRect;
    UiRect PauseMainMenuRect;
''',
    '''    UiRect PauseMapRect;
    UiRect PauseResumeRect;
    UiRect PauseMainMenuRect;
    UiRect FullMapRect;
    UiRect FullMapBackRect;
    UiRect FullMapRandomRect;
    UiRect FullMapClearRect;
''',
    "Renderer map rects",
)
renderer_h = replace_once(
    renderer_h,
    '''    float PauseMapHover = 0.0f;
    float PauseResumeHover = 0.0f;
    float PauseMainMenuHover = 0.0f;
''',
    '''    float PauseMapHover = 0.0f;
    float PauseResumeHover = 0.0f;
    float PauseMainMenuHover = 0.0f;
    float FullMapBackHover = 0.0f;
    float FullMapRandomHover = 0.0f;
    float FullMapClearHover = 0.0f;
''',
    "Renderer map hover fields",
)
write("src/Rendering/Renderer.h", renderer_h)

map_overlay = r'''#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

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

    float FullMapScale(float Zoom)
    {
        return 7.2f * std::clamp(Zoom, 0.45f, 4.0f);
    }

    std::string MarkerName(MapMarkerKind Kind)
    {
        if (Kind == MapMarkerKind::Breaker)
            return "BREAKER";
        if (Kind == MapMarkerKind::BreakerActive)
            return "BREAKER ONLINE";
        if (Kind == MapMarkerKind::Exit)
            return "EXIT - NO POWER";
        if (Kind == MapMarkerKind::ExitPowered)
            return "POWERED EXIT";
        return "ENTITY";
    }
}

void Renderer::DrawMapPanelV2(
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
    const MapWaypointView& Waypoint,
    const std::vector<glm::vec2>& Route,
    bool Detailed,
    float Time
)
{
    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 WallpaperA{0.748f, 0.696f, 0.346f};
    const glm::vec3 WallpaperB{0.790f, 0.735f, 0.366f};
    const glm::vec3 Wall{0.125f, 0.113f, 0.050f};
    const glm::vec3 RouteColor{0.98f, 0.95f, 0.72f};

    auto ClipRect = [&](int RectX, int RectY, int RectWidth, int RectHeight, const glm::vec3& Color)
    {
        const int Left = std::max(RectX, X);
        const int Top = std::max(RectY, Y);
        const int Right = std::min(RectX + RectWidth, X + PanelWidth);
        const int Bottom = std::min(RectY + RectHeight, Y + PanelHeight);

        if (Right > Left && Bottom > Top)
            DrawRect(Left, Top, Right - Left, Bottom - Top, Color);
    };

    auto FillCircle = [&](int CenterX, int CenterY, int Radius, const glm::vec3& Color)
    {
        for (int OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
        {
            const float Inside =
                static_cast<float>(Radius * Radius - OffsetY * OffsetY);
            const int HalfWidth =
                Inside > 0.0f
                    ? static_cast<int>(std::sqrt(Inside))
                    : 0;

            ClipRect(
                CenterX - HalfWidth,
                CenterY + OffsetY,
                HalfWidth * 2 + 1,
                1,
                Color
            );
        }
    };

    auto Ring = [&](int CenterX, int CenterY, int Radius, int Thickness, const glm::vec3& Color)
    {
        FillCircle(CenterX, CenterY, Radius, Color);
        FillCircle(
            CenterX,
            CenterY,
            std::max(Radius - Thickness, 0),
            Yellow
        );
    };

    auto Line = [&](glm::ivec2 A, glm::ivec2 B, int Thickness, const glm::vec3& Color)
    {
        const int DeltaX = B.x - A.x;
        const int DeltaY = B.y - A.y;
        const int Steps = std::max(std::abs(DeltaX), std::abs(DeltaY));

        if (Steps <= 0)
        {
            ClipRect(A.x, A.y, Thickness, Thickness, Color);
            return;
        }

        for (int I = 0; I <= Steps; ++I)
        {
            const float T = static_cast<float>(I) / static_cast<float>(Steps);
            const int DrawX = static_cast<int>(std::round(A.x + DeltaX * T));
            const int DrawY = static_cast<int>(std::round(A.y + DeltaY * T));
            ClipRect(
                DrawX - Thickness / 2,
                DrawY - Thickness / 2,
                Thickness,
                Thickness,
                Color
            );
        }
    };

    DrawRect(X, Y, PanelWidth, PanelHeight, Yellow);

    for (int StripeX = X + 17; StripeX < X + PanelWidth; StripeX += 34)
        ClipRect(StripeX, Y, 2, PanelHeight, WallpaperA);

    for (int StripeX = X + 26; StripeX < X + PanelWidth; StripeX += 68)
        ClipRect(StripeX, Y, 1, PanelHeight, WallpaperB);

    for (int StripeY = Y + 23; StripeY < Y + PanelHeight; StripeY += 46)
    {
        for (int MotifX = X + 8; MotifX < X + PanelWidth; MotifX += 34)
        {
            ClipRect(MotifX, StripeY, 7, 1, WallpaperB);
            ClipRect(MotifX + 3, StripeY - 2, 1, 5, WallpaperA);
        }
    }

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
            const float CenterX = static_cast<float>(Cell.X) * World.CellSize;
            const float CenterZ = static_cast<float>(Cell.Z) * World.CellSize;
            const glm::ivec2 TopLeft = ToScreen(CenterX - HalfCell, CenterZ - HalfCell);
            const glm::ivec2 BottomRight = ToScreen(CenterX + HalfCell, CenterZ + HalfCell);

            if (
                BottomRight.x < X ||
                BottomRight.y < Y ||
                TopLeft.x > X + PanelWidth ||
                TopLeft.y > Y + PanelHeight
            )
            {
                continue;
            }

            const int CellWidth = std::max(BottomRight.x - TopLeft.x, 1);
            const int CellHeight = std::max(BottomRight.y - TopLeft.y, 1);

            if (Detailed && CellWidth > 5 && CellHeight > 5)
            {
                const int TintIndex = std::abs(Cell.X * 29 + Cell.Z * 43) % 4;
                const glm::vec3 Tint =
                    TintIndex == 0
                        ? glm::vec3{0.785f, 0.730f, 0.360f}
                        : TintIndex == 1
                            ? glm::vec3{0.800f, 0.744f, 0.378f}
                            : TintIndex == 2
                                ? glm::vec3{0.765f, 0.710f, 0.340f}
                                : glm::vec3{0.810f, 0.755f, 0.390f};

                ClipRect(
                    TopLeft.x + 1,
                    TopLeft.y + 1,
                    std::max(CellWidth - 2, 1),
                    std::max(CellHeight - 2, 1),
                    Tint
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

    if (Route.size() >= 2)
    {
        for (std::size_t I = 1; I < Route.size(); ++I)
        {
            const glm::ivec2 A = ToScreen(Route[I - 1].x, Route[I - 1].y);
            const glm::ivec2 B = ToScreen(Route[I].x, Route[I].y);
            const glm::vec2 Segment = glm::vec2(B - A);
            const float Length = glm::length(Segment);
            const int DotCount = std::max(static_cast<int>(Length / (Detailed ? 9.0f : 7.0f)), 1);

            for (int Dot = 0; Dot <= DotCount; ++Dot)
            {
                const float T = static_cast<float>(Dot) / static_cast<float>(DotCount);
                const int DotX = static_cast<int>(std::round(A.x + (B.x - A.x) * T));
                const int DotY = static_cast<int>(std::round(A.y + (B.y - A.y) * T));
                const int Size = Detailed ? 4 : 3;
                ClipRect(DotX - Size / 2, DotY - Size / 2, Size, Size, RouteColor);
            }
        }
    }

    float HoverDistance = std::numeric_limits<float>::max();
    std::string HoverLabel;
    glm::ivec2 HoverScreen{0};

    for (const MapMarker& Marker : Markers)
    {
        const glm::ivec2 Position = ToScreen(Marker.Position.x, Marker.Position.y);

        if (
            Position.x < X - 18 ||
            Position.y < Y - 18 ||
            Position.x > X + PanelWidth + 18 ||
            Position.y > Y + PanelHeight + 18
        )
        {
            continue;
        }

        if (Marker.Kind == MapMarkerKind::Breaker || Marker.Kind == MapMarkerKind::BreakerActive)
        {
            const bool Active = Marker.Kind == MapMarkerKind::BreakerActive;
            const glm::vec3 Glow = Active
                ? glm::vec3{0.14f, 0.95f, 0.30f}
                : glm::vec3{1.0f, 0.10f, 0.055f};
            const int Radius = Detailed ? 9 : 7;
            Ring(Position.x, Position.y, Radius + 2, 2, Ink);
            Ring(Position.x, Position.y, Radius, 3, Glow);
            Line(
                {Position.x - 3, Position.y + 4},
                {Position.x + 4, Position.y - 4},
                2,
                Glow
            );
            ClipRect(Position.x + 3, Position.y - 6, 5, 3, Glow);
            ClipRect(Position.x - 5, Position.y + 3, 4, 4, Glow);
        }
        else if (Marker.Kind == MapMarkerKind::Exit || Marker.Kind == MapMarkerKind::ExitPowered)
        {
            const glm::vec3 Color =
                Marker.Kind == MapMarkerKind::ExitPowered
                    ? glm::vec3{0.18f, 0.98f, 0.46f}
                    : glm::vec3{0.96f, 0.93f, 0.73f};
            const int Size = Detailed ? 13 : 10;
            ClipRect(Position.x - Size / 2 - 2, Position.y - Size / 2 - 2, Size + 4, Size + 4, Ink);
            ClipRect(Position.x - Size / 2, Position.y - Size / 2, Size, Size, Color);
            ClipRect(Position.x + Size / 4, Position.y, 2, 2, Ink);
        }
        else
        {
            const bool RedPhase = std::fmod(Time * 6.0f, 1.0f) < 0.55f;
            const glm::vec3 Threat = RedPhase
                ? glm::vec3{1.0f, 0.03f, 0.02f}
                : Ink;
            const int Radius = Detailed ? 8 : 6;
            FillCircle(Position.x, Position.y, Radius + 3, Ink);
            FillCircle(Position.x, Position.y, Radius, Threat);
            ClipRect(Position.x - 1, Position.y - Radius - 5, 3, 4, Threat);
        }

        if (Detailed)
        {
            const float DeltaX = static_cast<float>(Position.x) - MenuPointerX;
            const float DeltaY = static_cast<float>(Position.y) - MenuPointerY;
            const float Distance = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);

            if (Distance < 18.0f && Distance < HoverDistance)
            {
                HoverDistance = Distance;
                const float Meters = glm::distance(PlayerPosition, Marker.Position);
                std::ostringstream Label;
                Label << MarkerName(Marker.Kind) << "   " << std::fixed << std::setprecision(0) << Meters << " M";
                HoverLabel = Label.str();
                HoverScreen = Position;
            }
        }
    }

    if (Waypoint.Active)
    {
        const glm::ivec2 Position = ToScreen(Waypoint.Position.x, Waypoint.Position.y);
        const int Pulse = 9 + static_cast<int>((std::sin(Time * 5.0f) + 1.0f) * 2.0f);
        Ring(Position.x, Position.y, Pulse + 3, 2, Ink);
        Ring(Position.x, Position.y, Pulse, 2, RouteColor);
        Line({Position.x - 5, Position.y}, {Position.x + 5, Position.y}, 2, RouteColor);
        Line({Position.x, Position.y - 5}, {Position.x, Position.y + 5}, 2, RouteColor);

        if (Detailed)
        {
            const float DeltaX = static_cast<float>(Position.x) - MenuPointerX;
            const float DeltaY = static_cast<float>(Position.y) - MenuPointerY;
            const float Distance = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);

            if (Distance < 20.0f && Distance < HoverDistance)
            {
                std::ostringstream Label;
                Label << "WAYPOINT   " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";
                HoverLabel = Label.str();
                HoverScreen = Position;
            }
        }
    }

    const glm::ivec2 PlayerScreen = ToScreen(PlayerPosition.x, PlayerPosition.y);
    glm::vec2 Direction = PlayerForward;

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const glm::vec2 Right{Direction.y, -Direction.x};
    const int ArrowLength = Detailed ? 13 : 10;
    const int ArrowWidth = Detailed ? 7 : 5;
    const glm::ivec2 Tip{
        PlayerScreen.x + static_cast<int>(std::round(Direction.x * ArrowLength)),
        PlayerScreen.y + static_cast<int>(std::round(Direction.y * ArrowLength))
    };
    const glm::ivec2 Left{
        PlayerScreen.x - static_cast<int>(std::round(Direction.x * 5.0f)) + static_cast<int>(std::round(Right.x * ArrowWidth)),
        PlayerScreen.y - static_cast<int>(std::round(Direction.y * 5.0f)) + static_cast<int>(std::round(Right.y * ArrowWidth))
    };
    const glm::ivec2 RightPoint{
        PlayerScreen.x - static_cast<int>(std::round(Direction.x * 5.0f)) - static_cast<int>(std::round(Right.x * ArrowWidth)),
        PlayerScreen.y - static_cast<int>(std::round(Direction.y * 5.0f)) - static_cast<int>(std::round(Right.y * ArrowWidth))
    };

    Line(Tip, Left, 5, Ink);
    Line(Tip, RightPoint, 5, Ink);
    Line(Left, RightPoint, 5, Ink);
    Line(Tip, Left, 2, {1.0f, 0.98f, 0.82f});
    Line(Tip, RightPoint, 2, {1.0f, 0.98f, 0.82f});
    Line(Left, RightPoint, 2, {1.0f, 0.98f, 0.82f});

    if (Detailed && !HoverLabel.empty() && GameplayTextRenderer.IsReady())
    {
        const int FontSize = 11;
        const int TextWidth = GameplayTextRenderer.Measure(HoverLabel, FontSize, 800, 0.05f);
        int LabelX = HoverScreen.x + 16;
        int LabelY = HoverScreen.y - 28;

        if (LabelX + TextWidth + 20 > X + PanelWidth)
            LabelX = HoverScreen.x - TextWidth - 28;
        if (LabelY < Y + 8)
            LabelY = HoverScreen.y + 18;

        DrawRect(LabelX - 8, LabelY - 6, TextWidth + 16, FontSize + 14, Ink);
        GameplayTextRenderer.Draw(
            HoverLabel,
            LabelX,
            LabelY,
            FontSize,
            800,
            0.05f,
            {0.98f, 0.95f, 0.74f},
            1.0f,
            false
        );
    }

    DrawRect(X, Y, PanelWidth, 2, Ink);
    DrawRect(X, Y + PanelHeight - 2, PanelWidth, 2, Ink);
    DrawRect(X, Y, 2, PanelHeight, Ink);
    DrawRect(X + PanelWidth - 2, Y, 2, PanelHeight, Ink);
}

void Renderer::DrawMiniMapV2(
    const WorldData& World,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    const MapWaypointView& Waypoint,
    const std::vector<glm::vec2>& Route,
    float ThreatDistance,
    float Time
)
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    const bool Compact = Width < 900 || Height < 620;
    const int MapWidth = std::clamp(static_cast<int>(Width * (Compact ? 0.26f : 0.18f)), 190, 300);
    const int MapHeight = std::clamp(static_cast<int>(Height * (Compact ? 0.23f : 0.20f)), 124, 190);
    const int Margin = std::clamp(static_cast<int>(Width * 0.016f), 14, 28);
    const int MapX = Margin;
    const int MapY = static_cast<int>(Height) - MapHeight - Margin;

    DrawMapPanelV2(
        World,
        MapX,
        MapY,
        MapWidth,
        MapHeight,
        PlayerPosition,
        Compact ? 1.38f : 1.58f,
        PlayerPosition,
        PlayerForward,
        Markers,
        Waypoint,
        Route,
        false,
        Time
    );

    const glm::vec3 Ink = LevelZeroInk();
    const bool ThreatNear = std::isfinite(ThreatDistance) && ThreatDistance < 36.0f;

    if (ThreatNear)
    {
        const bool RedPhase = std::fmod(Time * 7.0f, 1.0f) < 0.5f;
        const glm::vec3 Alert = RedPhase
            ? glm::vec3{0.98f, 0.025f, 0.015f}
            : glm::vec3{0.015f, 0.012f, 0.008f};
        const int Border = RedPhase ? 5 : 4;
        DrawRect(MapX - Border, MapY - Border, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY + MapHeight, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY, Border, MapHeight, Alert);
        DrawRect(MapX + MapWidth, MapY, Border, MapHeight, Alert);
    }

    if (GameplayTextRenderer.IsReady())
    {
        GameplayTextRenderer.Resize(Width, Height);
        GameplayTextRenderer.Draw(
            ThreatNear ? "THREAT NEAR" : "M  MAP",
            MapX + 10,
            MapY + 9,
            9,
            850,
            0.09f,
            ThreatNear ? glm::vec3{0.78f, 0.025f, 0.015f} : Ink,
            1.0f,
            false
        );

        if (Waypoint.Active)
        {
            std::ostringstream Distance;
            Distance << "WAYPOINT  " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";
            GameplayTextRenderer.Draw(
                Distance.str(),
                MapX + 10,
                MapY + MapHeight - 21,
                9,
                800,
                0.05f,
                Ink,
                1.0f,
                false
            );
        }
    }
}

void Renderer::DrawFullMapV2(
    const WorldData& World,
    const glm::vec2& MapCenter,
    float Zoom,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    const MapWaypointView& Waypoint,
    const std::vector<glm::vec2>& Route,
    int BreakersActive,
    int BreakersRequired,
    float Time
)
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    GameplayTextRenderer.Resize(Width, Height);
    glDisable(GL_DEPTH_TEST);

    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 Muted{0.36f, 0.32f, 0.14f};
    const glm::vec3 ButtonHover{0.69f, 0.63f, 0.28f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Yellow);

    for (int X = 15; X < static_cast<int>(Width); X += 38)
        DrawRect(X, 0, 1, static_cast<int>(Height), {0.745f, 0.690f, 0.337f});

    const bool Compact = Width < 930 || Height < 650;
    const int Margin = Compact ? 14 : 28;
    const int ButtonHeight = Compact ? 34 : 40;
    const int HeaderHeight = Compact ? 94 : 108;
    const int ButtonY = 16;
    const int Gap = Compact ? 7 : 10;

    const int BackWidth = Compact ? 112 : 142;
    const int RandomWidth = Compact ? 148 : 190;
    const int ClearWidth = Compact ? 116 : 148;

    FullMapBackRect = {Margin, ButtonY, BackWidth, ButtonHeight};
    FullMapClearRect = {
        static_cast<int>(Width) - Margin - ClearWidth,
        ButtonY,
        ClearWidth,
        ButtonHeight
    };
    FullMapRandomRect = {
        FullMapClearRect.X - Gap - RandomWidth,
        ButtonY,
        RandomWidth,
        ButtonHeight
    };

    auto DrawButton = [&](const UiRect& Rect, const std::string& Label, float Hover)
    {
        const glm::vec3 Fill = Hover > 0.01f ? ButtonHover : Yellow;
        DrawRect(Rect.X, Rect.Y, Rect.Width, Rect.Height, Fill);
        DrawRect(Rect.X, Rect.Y, Rect.Width, 2, Ink);
        DrawRect(Rect.X, Rect.Y + Rect.Height - 2, Rect.Width, 2, Ink);
        DrawRect(Rect.X, Rect.Y, 2, Rect.Height, Ink);
        DrawRect(Rect.X + Rect.Width - 2, Rect.Y, 2, Rect.Height, Ink);

        GameplayTextRenderer.Draw(
            Label,
            Rect.X + 12,
            Rect.Y + (Rect.Height - 11) / 2,
            11,
            850,
            0.06f,
            Ink,
            1.0f,
            false
        );
    };

    DrawButton(FullMapBackRect, "<  BACK", FullMapBackHover);
    DrawButton(FullMapRandomRect, "RANDOM DESTINATION", FullMapRandomHover);
    DrawButton(FullMapClearRect, "CLEAR MARKER", FullMapClearHover);

    const int TitleY = ButtonY + ButtonHeight + (Compact ? 10 : 12);
    GameplayTextRenderer.Draw(
        "LEVEL 0 MAP",
        Margin,
        TitleY,
        Compact ? 20 : 25,
        900,
        0.04f,
        Ink,
        1.0f,
        false
    );

    std::ostringstream Status;
    Status << "POWER " << BreakersActive << "/" << BreakersRequired;

    if (Waypoint.Active)
        Status << "     WAYPOINT " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";

    const int StatusWidth = GameplayTextRenderer.Measure(Status.str(), 10, 800, 0.08f);
    GameplayTextRenderer.Draw(
        Status.str(),
        std::max(Margin, static_cast<int>(Width) - Margin - StatusWidth),
        TitleY + 5,
        10,
        800,
        0.08f,
        Muted,
        1.0f,
        false
    );

    const int FooterHeight = Compact ? 52 : 60;
    FullMapRect = {
        Margin,
        HeaderHeight,
        std::max(static_cast<int>(Width) - Margin * 2, 1),
        std::max(static_cast<int>(Height) - HeaderHeight - FooterHeight, 1)
    };

    DrawMapPanelV2(
        World,
        FullMapRect.X,
        FullMapRect.Y,
        FullMapRect.Width,
        FullMapRect.Height,
        MapCenter,
        FullMapScale(Zoom),
        PlayerPosition,
        PlayerForward,
        Markers,
        Waypoint,
        Route,
        true,
        Time
    );

    std::ostringstream ZoomText;
    ZoomText << "ZOOM " << std::fixed << std::setprecision(0) << std::clamp(Zoom, 0.45f, 4.0f) * 100.0f << "%";

    const int FooterY = static_cast<int>(Height) - FooterHeight + 11;
    GameplayTextRenderer.Draw(
        "ESC BACK TO MENU   |   CLICK SET MARKER   |   DRAG PAN   |   WHEEL ZOOM   |   R RECENTER   |   G RANDOM",
        Margin,
        FooterY,
        Compact ? 8 : 9,
        720,
        0.045f,
        Muted,
        1.0f,
        false
    );

    GameplayTextRenderer.Draw(
        "HOVER ICONS FOR LABELS   |   WAYPOINT CLEARS WHEN REACHED",
        Margin,
        FooterY + 17,
        Compact ? 8 : 9,
        650,
        0.04f,
        Muted,
        0.9f,
        false
    );

    const int ZoomWidth = GameplayTextRenderer.Measure(ZoomText.str(), 9, 800, 0.06f);
    GameplayTextRenderer.Draw(
        ZoomText.str(),
        static_cast<int>(Width) - Margin - ZoomWidth,
        FooterY + 17,
        9,
        800,
        0.06f,
        Ink,
        1.0f,
        false
    );

    glEnable(GL_DEPTH_TEST);
}

MapUiAction Renderer::HitTestFullMap() const
{
    if (FullMapBackRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::Back;
    if (FullMapRandomRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::RandomDestination;
    if (FullMapClearRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::ClearWaypoint;
    return MapUiAction::None;
}

bool Renderer::PointerInsideFullMap() const
{
    return FullMapRect.Contains(MenuPointerX, MenuPointerY);
}

glm::vec2 Renderer::FullMapPointerToWorld(
    const glm::vec2& MapCenter,
    float Zoom
) const
{
    const float Scale = FullMapScale(Zoom);
    return {
        MapCenter.x +
            (MenuPointerX - static_cast<float>(FullMapRect.X + FullMapRect.Width / 2)) / Scale,
        MapCenter.y +
            (MenuPointerY - static_cast<float>(FullMapRect.Y + FullMapRect.Height / 2)) / Scale
    };
}
'''
write("src/Rendering/MapOverlay.cpp", map_overlay)

touch_controls = r'''#include "Renderer.h"

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
'''
write("src/Rendering/TouchControls.cpp", touch_controls)

interactive = read("src/Rendering/InteractiveMenu.cpp")
interactive = replace_once(
    interactive,
    '''    MoveToward(
        PauseMainMenuHover,
        PauseMainMenuRect.Contains(MenuPointerX, MenuPointerY)
    );
''',
    '''    MoveToward(
        PauseMainMenuHover,
        PauseMainMenuRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        FullMapBackHover,
        FullMapBackRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        FullMapRandomHover,
        FullMapRandomRect.Contains(MenuPointerX, MenuPointerY)
    );

    MoveToward(
        FullMapClearHover,
        FullMapClearRect.Contains(MenuPointerX, MenuPointerY)
    );
''',
    "full map hover animations",
)
interactive = interactive.replace('"ESC PAUSE   M MAP"', '"ESC PAUSE   M MAP   F11 FULLSCREEN"')
interactive = interactive.replace(
    '"MAP OPENS THE INFINITE LEVEL 0 GRID. PAN FOREVER, TRACK BREAKERS, EXIT AND THREATS."',
    '"MAP IS A PAUSE-MENU TAB. PAN IN ANY DIRECTION, SET ROUTED WAYPOINTS, TRACK BREAKERS, EXIT AND THREATS."',
)
interactive = interactive.replace('"M MAP     ESC RESUME"', '"M MAP     ESC RESUME     F11 FULLSCREEN"')
write("src/Rendering/InteractiveMenu.cpp", interactive)

app_h = r'''#pragma once

#include "../Game/Game.h"
#include "../Updater/UpdaterService.h"

#include <SDL3/SDL.h>

#include <cstdint>

class Application
{
public:
    Application() = default;
    ~Application();

    bool Initialize();
    int Run();

private:
    bool InitializeWindow();
    bool InitializeOpenGL();

    void ProcessEvents();
    void HandleResize();
    void SetMouseCaptured(bool Captured);
    void ToggleFullscreen();
    void Shutdown();

    SDL_Window* Window = nullptr;
    SDL_GLContext GLContext = nullptr;

    Game Backrooms;
    UpdaterService Updater;

    bool Running = false;
    bool MouseCaptured = false;
    bool Initialized = false;
    bool Fullscreen = false;

    uint32_t Width = 1600;
    uint32_t Height = 900;

    uint64_t LastCounter = 0;
    double CounterFrequency = 1.0;
    float TotalTime = 0.0f;
};
'''
write("src/Core/Application.h", app_h)

app_cpp = read("src/Core/Application.cpp")
set_mouse_anchor = '''void Application::HandleResize()
{
'''
toggle = '''void Application::ToggleFullscreen()
{
    if (Window == nullptr)
        return;

    const bool Requested = !Fullscreen;

    if (!SDL_SetWindowFullscreen(Window, Requested))
    {
        SDL_Log("Fullscreen toggle failed: %s", SDL_GetError());
        return;
    }

    Fullscreen = Requested;
    HandleResize();
}

void Application::HandleResize()
{
'''
app_cpp = replace_once(app_cpp, set_mouse_anchor, toggle, "fullscreen method")
resize_block = '''        if (
            Event.type ==
            SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
        )
        {
            HandleResize();
        }

        const bool UpdateActive =
'''
resize_new = '''        if (
            Event.type ==
            SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
        )
        {
            HandleResize();
        }

        if (
            Event.type == SDL_EVENT_KEY_DOWN &&
            !Event.key.repeat &&
            Event.key.scancode == SDL_SCANCODE_F11
        )
        {
            ToggleFullscreen();
            continue;
        }

        const bool UpdateActive =
'''
app_cpp = replace_once(app_cpp, resize_block, resize_new, "F11 event")
write("src/Core/Application.cpp", app_cpp)

game_h = r'''#pragma once

#include "../Audio/AudioSystem.h"
#include "../Entity/Entity.h"
#include "../Player/Player.h"
#include "../Rendering/Renderer.h"
#include "../World/WorldGenerator.h"
#include "GameState.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct UpdateVisualState;

class Game
{
public:
    bool Initialize(uint32_t Width, uint32_t Height);
    void Shutdown();

    void Resize(uint32_t Width, uint32_t Height);

    void HandleEvent(
        const SDL_Event& Event,
        bool MouseCaptured
    );

    void OnMouseCaptureChanged(bool Captured);
    bool ShouldCaptureMouse() const;

    bool HasMenuOverlay() const;
    void RenderMenuOverlay();

    void Update(
        float DeltaTime,
        bool MouseCaptured
    );

    void Render(float Time);
    void RenderUpdateScreen(const UpdateVisualState& State);
    void RenderUpdateScreenV2(const UpdateVisualState& State);

    const std::string& WindowTitle() const;

private:
    struct Breaker
    {
        glm::vec3 Position{0.0f};
        glm::vec3 Forward{0.0f, 0.0f, 1.0f};
        bool Active = false;
    };

    void Reset();
    std::vector<MapMarker> BuildMapMarkers() const;
    void OpenMap(bool ReturnToPause);
    void CloseMap(bool ForcePauseMenu = false);
    glm::vec3 MenuPointerFromEvent(const SDL_Event& Event) const;

    void SetWaypoint(const glm::vec2& Position);
    void SetRandomWaypoint();
    void ClearWaypoint();
    void RebuildWaypointPath();
    void UpdateWaypoint(float DeltaTime);
    float CurrentWaypointDistance() const;

    bool TryMountBreaker(
        const glm::vec3& CellCenter,
        int Variant,
        Breaker& Result
    ) const;

    void UpdateInteraction();
    void Interact();
    void EndGame(bool Escaped);

    AABB BreakerBounds(const Breaker& BreakerData) const;
    AABB ExitBounds() const;

    std::vector<SceneBox> BuildDynamicBoxes() const;
    void UpdateTitle();

    uint32_t Width = 1600;
    uint32_t Height = 900;
    uint32_t Seed = 1;

    Renderer GameRenderer;
    WorldData World;
    Player GamePlayer;
    Entity Hunter;
    AudioSystem Audio;
    GameState State;

    std::vector<Breaker> Breakers;
    glm::vec3 ExitPosition{0.0f};
    glm::vec3 ExitForward{0.0f, 0.0f, 1.0f};

    int InteractionType = 0;
    int InteractionIndex = -1;

    bool InteractPressed = false;
    bool RestartPressed = false;

    bool MapOpen = false;
    bool MapReturnToPause = false;
    bool MapDragging = false;
    bool MapTouchDragging = false;
    SDL_FingerID MapTouchFinger = 0;
    glm::vec2 MapCenter{0.0f};
    glm::vec2 MapDragPointer{0.0f};
    float MapDragDistance = 0.0f;
    float MapZoom = 1.0f;

    bool WaypointActive = false;
    glm::vec2 WaypointPosition{0.0f};
    std::vector<glm::vec2> WaypointPath;
    int WaypointRouteCellX = 0;
    int WaypointRouteCellZ = 0;
    int RandomWaypointCounter = 0;
    float WaypointRepathTimer = 0.0f;

    int FrameCounter = 0;
    uint64_t FpsCounterStart = 0;
    float DisplayedFps = 0.0f;

    glm::vec3 PreviousPlayerPosition{0.0f};
    float FootstepDistance = 0.0f;

    std::string Message;
    float MessageTimer = 0.0f;

    std::string Title = "Backrooms Offical";
};
'''
write("src/Game/Game.h", game_h)

game_cpp = read("src/Game/Game.cpp")
game_cpp = replace_once(
    game_cpp,
    '#include "Game.h"\n\n#include "../Physics/Raycast.h"',
    '#include "Game.h"\n#include "MapNavigation.h"\n\n#include "../Physics/Raycast.h"',
    "Game MapNavigation include",
)
game_cpp = replace_once(
    game_cpp,
    '''void Game::Shutdown()
{
    Audio.Shutdown();
    GameRenderer.ShutdownInterfaceV3();
    GameRenderer.Shutdown();
}
''',
    '''void Game::Shutdown()
{
    GamePlayer.ShutdownInput();
    Audio.Shutdown();
    GameRenderer.ShutdownInterfaceV3();
    GameRenderer.Shutdown();
}
''',
    "Game shutdown input",
)

render_menu_pattern = re.compile(r"void Game::RenderMenuOverlay\(\)\n\{.*?\n\}\n\nglm::vec3 Game::MenuPointerFromEvent", re.S)
render_menu_replacement = '''void Game::RenderMenuOverlay()
{
    GameRenderer.BeginFrame();

    if (MapOpen)
    {
        const float Scale = 7.2f * std::clamp(MapZoom, 0.45f, 4.0f);
        const float HalfWidthMeters = static_cast<float>(Width) * 0.5f / Scale;
        const float HalfHeightMeters = static_cast<float>(Height) * 0.5f / Scale;
        const float ChunkMeters = std::max(World.CellSize * static_cast<float>(World.ChunkCells), 1.0f);
        const int Radius = std::clamp(
            static_cast<int>(std::ceil(std::max(HalfWidthMeters, HalfHeightMeters) / ChunkMeters)) + 2,
            2,
            12
        );

        WorldGenerator Generator(Seed);
        const WorldData MapWorld = Generator.BuildMapRegion(
            {MapCenter.x, 0.0f, MapCenter.y},
            Radius
        );

        MapWaypointView Waypoint;
        Waypoint.Active = WaypointActive;
        Waypoint.Position = WaypointPosition;
        Waypoint.DistanceMeters = CurrentWaypointDistance();

        const float Time = static_cast<float>(SDL_GetTicksNS()) / 1000000000.0f;

        GameRenderer.DrawFullMapV2(
            MapWorld,
            MapCenter,
            MapZoom,
            {GamePlayer.Position().x, GamePlayer.Position().z},
            {GamePlayer.Forward().x, GamePlayer.Forward().z},
            BuildMapMarkers(),
            Waypoint,
            WaypointPath,
            State.BreakersActive,
            State.BreakersRequired,
            Time
        );
        return;
    }

    if (State.Paused)
        GameRenderer.DrawPauseMenuV3();
    else
        GameRenderer.DrawMainMenuV3(State.Started);
}

glm::vec3 Game::MenuPointerFromEvent'''
game_cpp, count = render_menu_pattern.subn(render_menu_replacement, game_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace RenderMenuOverlay")

open_close_pattern = re.compile(r"void Game::OpenMap\(bool ReturnToPause\)\n\{.*?\n\}\n\nvoid Game::CloseMap\(\)\n\{.*?\n\}\n", re.S)
open_close_replacement = '''void Game::OpenMap(bool ReturnToPause)
{
    MapOpen = true;
    MapReturnToPause = ReturnToPause;
    MapDragging = false;
    MapTouchDragging = false;
    MapTouchFinger = 0;
    MapDragDistance = 0.0f;
    MapZoom = std::clamp(MapZoom, 0.45f, 4.0f);
    MapCenter = {
        GamePlayer.Position().x,
        GamePlayer.Position().z
    };

    State.Paused = false;
    State.MainMenuOpen = false;
    GameRenderer.ClearMenuPointer();
}

void Game::CloseMap(bool ForcePauseMenu)
{
    const bool ReturnToPause = MapReturnToPause || ForcePauseMenu;

    MapOpen = false;
    MapReturnToPause = false;
    MapDragging = false;
    MapTouchDragging = false;
    MapTouchFinger = 0;
    MapDragDistance = 0.0f;
    GameRenderer.ClearMenuPointer();

    if (ReturnToPause)
    {
        State.Paused = true;
        State.MainMenuOpen = false;
    }
}

float Game::CurrentWaypointDistance() const
{
    if (!WaypointActive)
        return 0.0f;

    return glm::distance(
        glm::vec2{GamePlayer.Position().x, GamePlayer.Position().z},
        WaypointPosition
    );
}

void Game::ClearWaypoint()
{
    WaypointActive = false;
    WaypointPath.clear();
    WaypointRepathTimer = 0.0f;
}

void Game::SetWaypoint(const glm::vec2& Position)
{
    const float CellSize = std::max(World.CellSize, 1.0f);
    WaypointPosition = {
        std::round(Position.x / CellSize) * CellSize,
        std::round(Position.y / CellSize) * CellSize
    };
    WaypointActive = true;
    RebuildWaypointPath();
    Message = "WAYPOINT SET";
    MessageTimer = 1.2f;
}

void Game::SetRandomWaypoint()
{
    WorldGenerator Generator(Seed);
    const glm::vec2 PlayerPosition{GamePlayer.Position().x, GamePlayer.Position().z};
    WorldData Region = Generator.BuildMapRegion(
        {PlayerPosition.x, 0.0f, PlayerPosition.y},
        5
    );

    uint32_t Value =
        Seed ^
        static_cast<uint32_t>(++RandomWaypointCounter * 0x9E3779B9u);

    auto Next = [&]()
    {
        Value ^= Value << 13;
        Value ^= Value >> 17;
        Value ^= Value << 5;
        return Value;
    };

    glm::vec2 Best = PlayerPosition;
    float BestDistance = 0.0f;

    for (int Attempt = 0; Attempt < 600; ++Attempt)
    {
        const int LocalX = static_cast<int>(Next() % static_cast<uint32_t>(Region.Columns));
        const int LocalZ = static_cast<int>(Next() % static_cast<uint32_t>(Region.Rows));
        const MazeCell& Cell = Region.Cell(LocalX, LocalZ);
        const glm::vec2 Candidate{
            static_cast<float>(Cell.X) * Region.CellSize,
            static_cast<float>(Cell.Z) * Region.CellSize
        };
        const float Distance = glm::distance(PlayerPosition, Candidate);

        if (Distance >= 85.0f && Distance <= 285.0f)
        {
            SetWaypoint(Candidate);
            Message = "RANDOM DESTINATION SET";
            MessageTimer = 1.4f;
            return;
        }

        if (Distance > BestDistance)
        {
            BestDistance = Distance;
            Best = Candidate;
        }
    }

    SetWaypoint(Best);
}

void Game::RebuildWaypointPath()
{
    WaypointPath.clear();

    if (!WaypointActive)
        return;

    const glm::vec2 Start{GamePlayer.Position().x, GamePlayer.Position().z};
    const glm::vec2 Midpoint = (Start + WaypointPosition) * 0.5f;
    const float Span = std::max(
        std::abs(WaypointPosition.x - Start.x),
        std::abs(WaypointPosition.y - Start.y)
    );
    const float ChunkMeters = std::max(World.CellSize * static_cast<float>(World.ChunkCells), 1.0f);
    const int Radius = std::clamp(
        static_cast<int>(std::ceil(Span * 0.5f / ChunkMeters)) + 3,
        2,
        32
    );

    WorldGenerator Generator(Seed);
    const WorldData RouteWorld = Generator.BuildMapRegion(
        {Midpoint.x, 0.0f, Midpoint.y},
        Radius
    );

    WaypointPath = MapNavigation::FindPath(
        RouteWorld,
        Start,
        WaypointPosition
    );

    if (WaypointPath.empty())
    {
        WaypointPath.push_back(Start);
        WaypointPath.push_back(WaypointPosition);
    }

    WaypointRouteCellX = static_cast<int>(std::round(Start.x / std::max(World.CellSize, 1.0f)));
    WaypointRouteCellZ = static_cast<int>(std::round(Start.y / std::max(World.CellSize, 1.0f)));
    WaypointRepathTimer = 0.65f;
}

void Game::UpdateWaypoint(float DeltaTime)
{
    if (!WaypointActive)
        return;

    if (CurrentWaypointDistance() <= 3.2f)
    {
        ClearWaypoint();
        Message = "WAYPOINT REACHED";
        MessageTimer = 1.5f;
        return;
    }

    WaypointRepathTimer = std::max(0.0f, WaypointRepathTimer - DeltaTime);

    const float CellSize = std::max(World.CellSize, 1.0f);
    const int CurrentCellX = static_cast<int>(std::round(GamePlayer.Position().x / CellSize));
    const int CurrentCellZ = static_cast<int>(std::round(GamePlayer.Position().z / CellSize));

    if (
        WaypointRepathTimer <= 0.0f &&
        (
            CurrentCellX != WaypointRouteCellX ||
            CurrentCellZ != WaypointRouteCellZ
        )
    )
    {
        RebuildWaypointPath();
    }
}
'''
game_cpp, count = open_close_pattern.subn(open_close_replacement, game_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace map open/close")

reset_old = '''    MapOpen = false;
    MapReturnToPause = false;
    MapDragging = false;
    MapCenter = {0.0f, 0.0f};
    MapDragPointer = {0.0f, 0.0f};
    MapZoom = 1.0f;
'''
reset_new = '''    MapOpen = false;
    MapReturnToPause = false;
    MapDragging = false;
    MapTouchDragging = false;
    MapTouchFinger = 0;
    MapCenter = {0.0f, 0.0f};
    MapDragPointer = {0.0f, 0.0f};
    MapDragDistance = 0.0f;
    MapZoom = 1.0f;

    WaypointActive = false;
    WaypointPosition = {0.0f, 0.0f};
    WaypointPath.clear();
    WaypointRouteCellX = 0;
    WaypointRouteCellZ = 0;
    RandomWaypointCounter = 0;
    WaypointRepathTimer = 0.0f;
'''
game_cpp = replace_once(game_cpp, reset_old, reset_new, "reset map/waypoint state")

map_event_pattern = re.compile(r"\n    if \(MapOpen\)\n    \{.*?\n    \}\n\n    if \(State.MainMenuOpen\)", re.S)
map_event_replacement = r'''
    if (MapOpen)
    {
        if (PointerEvent)
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
        }

        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_ESCAPE
        )
        {
            CloseMap(true);
            return;
        }

        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_M
        )
        {
            CloseMap(false);
            return;
        }

        if (
            Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
            (
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_START ||
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST
            )
        )
        {
            CloseMap(true);
            return;
        }

        if (KeyDown && Event.key.scancode == SDL_SCANCODE_R)
        {
            MapCenter = {
                GamePlayer.Position().x,
                GamePlayer.Position().z
            };
            return;
        }

        if (KeyDown && Event.key.scancode == SDL_SCANCODE_G)
        {
            SetRandomWaypoint();
            return;
        }

        if (
            KeyDown &&
            (
                Event.key.scancode == SDL_SCANCODE_DELETE ||
                Event.key.scancode == SDL_SCANCODE_BACKSPACE
            )
        )
        {
            ClearWaypoint();
            return;
        }

        if (Event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            const float Factor = Event.wheel.y > 0.0f ? 1.12f : 0.89f;
            MapZoom = std::clamp(MapZoom * Factor, 0.45f, 4.0f);
            return;
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            Event.button.button == SDL_BUTTON_RIGHT &&
            GameRenderer.PointerInsideFullMap()
        )
        {
            SetWaypoint(GameRenderer.FullMapPointerToWorld(MapCenter, MapZoom));
            return;
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            Event.button.button == SDL_BUTTON_LEFT
        )
        {
            const MapUiAction Action = GameRenderer.HitTestFullMap();

            if (Action == MapUiAction::Back)
            {
                CloseMap(true);
                return;
            }

            if (Action == MapUiAction::RandomDestination)
            {
                SetRandomWaypoint();
                return;
            }

            if (Action == MapUiAction::ClearWaypoint)
            {
                ClearWaypoint();
                return;
            }

            if (GameRenderer.PointerInsideFullMap())
            {
                const glm::vec3 Pointer = MenuPointerFromEvent(Event);
                MapDragging = true;
                MapDragDistance = 0.0f;
                MapDragPointer = {Pointer.x, Pointer.y};
            }
            return;
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
            Event.button.button == SDL_BUTTON_LEFT
        )
        {
            if (
                MapDragging &&
                MapDragDistance <= 6.0f &&
                GameRenderer.PointerInsideFullMap()
            )
            {
                SetWaypoint(GameRenderer.FullMapPointerToWorld(MapCenter, MapZoom));
            }

            MapDragging = false;
            MapDragDistance = 0.0f;
            return;
        }

        if (Event.type == SDL_EVENT_MOUSE_MOTION && MapDragging)
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            const glm::vec2 CurrentPointer{Pointer.x, Pointer.y};
            const glm::vec2 Delta = CurrentPointer - MapDragPointer;
            const float PixelsPerMeter = 7.2f * std::max(MapZoom, 0.45f);

            MapCenter.x -= Delta.x / PixelsPerMeter;
            MapCenter.y -= Delta.y / PixelsPerMeter;
            MapDragDistance += glm::length(Delta);
            MapDragPointer = CurrentPointer;
            return;
        }

        if (Event.type == SDL_EVENT_FINGER_DOWN)
        {
            const glm::vec2 Pointer{
                std::clamp(Event.tfinger.x, 0.0f, 1.0f) * static_cast<float>(Width),
                std::clamp(Event.tfinger.y, 0.0f, 1.0f) * static_cast<float>(Height)
            };

            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
            const MapUiAction Action = GameRenderer.HitTestFullMap();

            if (Action == MapUiAction::Back)
            {
                CloseMap(true);
                return;
            }

            if (Action == MapUiAction::RandomDestination)
            {
                SetRandomWaypoint();
                return;
            }

            if (Action == MapUiAction::ClearWaypoint)
            {
                ClearWaypoint();
                return;
            }

            if (GameRenderer.PointerInsideFullMap())
            {
                MapTouchDragging = true;
                MapTouchFinger = Event.tfinger.fingerID;
                MapDragDistance = 0.0f;
                MapDragPointer = Pointer;
            }
            return;
        }

        if (
            Event.type == SDL_EVENT_FINGER_MOTION &&
            MapTouchDragging &&
            Event.tfinger.fingerID == MapTouchFinger
        )
        {
            const glm::vec2 Pointer{
                std::clamp(Event.tfinger.x, 0.0f, 1.0f) * static_cast<float>(Width),
                std::clamp(Event.tfinger.y, 0.0f, 1.0f) * static_cast<float>(Height)
            };
            const glm::vec2 Delta = Pointer - MapDragPointer;
            const float PixelsPerMeter = 7.2f * std::max(MapZoom, 0.45f);
            MapCenter.x -= Delta.x / PixelsPerMeter;
            MapCenter.y -= Delta.y / PixelsPerMeter;
            MapDragDistance += glm::length(Delta);
            MapDragPointer = Pointer;
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
            return;
        }

        if (
            (Event.type == SDL_EVENT_FINGER_UP || Event.type == SDL_EVENT_FINGER_CANCELED) &&
            MapTouchDragging &&
            Event.tfinger.fingerID == MapTouchFinger
        )
        {
            const glm::vec2 Pointer{
                std::clamp(Event.tfinger.x, 0.0f, 1.0f) * static_cast<float>(Width),
                std::clamp(Event.tfinger.y, 0.0f, 1.0f) * static_cast<float>(Height)
            };
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);

            if (MapDragDistance <= 10.0f && GameRenderer.PointerInsideFullMap())
                SetWaypoint(GameRenderer.FullMapPointerToWorld(MapCenter, MapZoom));

            MapTouchDragging = false;
            MapTouchFinger = 0;
            MapDragDistance = 0.0f;
            return;
        }

        return;
    }

    if (State.MainMenuOpen)'''
game_cpp, count = map_event_pattern.subn(map_event_replacement, game_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace map event block")

main_activate_anchor = '''        if (Activate)
        {
            State.Started = true;
            State.MainMenuOpen = false;
            State.Paused = false;
            GameRenderer.ClearMenuPointer();
        }
'''
main_activate_new = '''        if (
            Activate ||
            (
                Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                (
                    Event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH ||
                    Event.gbutton.button == SDL_GAMEPAD_BUTTON_START
                )
            )
        )
        {
            State.Started = true;
            State.MainMenuOpen = false;
            State.Paused = false;
            GameRenderer.ClearMenuPointer();
        }
'''
game_cpp = replace_once(game_cpp, main_activate_anchor, main_activate_new, "main gamepad activate")

pause_m_anchor = '''        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_M
        )
        {
            OpenMap(true);
            return;
        }

        if (
            Activate ||
'''
pause_m_new = '''        if (
            (
                KeyDown &&
                Event.key.scancode == SDL_SCANCODE_M
            ) ||
            (
                Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST
            )
        )
        {
            OpenMap(true);
            return;
        }

        if (
            Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
            (
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_START ||
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST
            )
        )
        {
            State.Paused = false;
            GameRenderer.ClearMenuPointer();
            return;
        }

        if (
            Activate ||
'''
game_cpp = replace_once(game_cpp, pause_m_anchor, pause_m_new, "pause gamepad controls")

gameplay_esc_anchor = '''    if (
        State.Started &&
        !State.Ended &&
        KeyDown &&
        Event.key.scancode == SDL_SCANCODE_ESCAPE
    )
    {
        State.Paused = true;
        GameRenderer.ClearMenuPointer();
        return;
    }
'''
gameplay_esc_new = '''    if (
        State.Started &&
        !State.Ended &&
        (
            (
                KeyDown &&
                Event.key.scancode == SDL_SCANCODE_ESCAPE
            ) ||
            (
                Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_START
            ) ||
            (
                Event.type == SDL_EVENT_FINGER_DOWN &&
                Event.tfinger.x > 0.82f &&
                Event.tfinger.y < 0.22f
            )
        )
    )
    {
        State.Paused = true;
        GameRenderer.ClearMenuPointer();
        return;
    }
'''
game_cpp = replace_once(game_cpp, gameplay_esc_anchor, gameplay_esc_new, "gameplay menu controls")

gameplay_map_anchor = '''    if (
        State.Started &&
        !State.Ended &&
        KeyDown &&
        Event.key.scancode == SDL_SCANCODE_M
    )
    {
        OpenMap(false);
        return;
    }

    GamePlayer.HandleEvent(Event, MouseCaptured);
'''
gameplay_map_new = '''    if (
        State.Started &&
        !State.Ended &&
        (
            (
                KeyDown &&
                Event.key.scancode == SDL_SCANCODE_M
            ) ||
            (
                Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST
            ) ||
            (
                Event.type == SDL_EVENT_FINGER_DOWN &&
                Event.tfinger.x > 0.66f &&
                Event.tfinger.x <= 0.82f &&
                Event.tfinger.y < 0.22f
            )
        )
    )
    {
        OpenMap(false);
        return;
    }

    if (
        State.Started &&
        !State.Ended &&
        (
            (
                Event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                Event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH
            ) ||
            (
                Event.type == SDL_EVENT_FINGER_DOWN &&
                Event.tfinger.x > 0.72f &&
                Event.tfinger.y > 0.72f
            )
        )
    )
    {
        InteractPressed = true;
    }

    GamePlayer.HandleEvent(Event, MouseCaptured);
'''
game_cpp = replace_once(game_cpp, gameplay_map_anchor, gameplay_map_new, "gameplay map/use device controls")

update_map_old = '''        if (glm::length(Pan) > 0.001f)
        {
            Pan = glm::normalize(Pan);
            const float PanSpeed =
                72.0f / std::max(MapZoom, 0.65f);
            MapCenter += Pan * PanSpeed * DeltaTime;
        }
'''
update_map_new = '''        if (glm::length(Pan) > 0.001f)
        {
            Pan = glm::normalize(Pan);
            const float PanSpeed =
                88.0f / std::max(MapZoom, 0.45f);
            MapCenter += Pan * PanSpeed * DeltaTime;
        }
'''
game_cpp = replace_once(game_cpp, update_map_old, update_map_new, "map pan speed")

player_update_anchor = '''    GamePlayer.Update(
        DeltaTime,
        World.Colliders,
        MouseCaptured
    );

    WorldGenerator Streamer(Seed);
'''
player_update_new = '''    GamePlayer.Update(
        DeltaTime,
        World.Colliders,
        MouseCaptured
    );

    UpdateWaypoint(DeltaTime);

    WorldGenerator Streamer(Seed);
'''
game_cpp = replace_once(game_cpp, player_update_anchor, player_update_new, "waypoint update")

mini_pattern = re.compile(r"        GameRenderer\.DrawMiniMapV1\(\n            World,.*?\n        \);", re.S)
mini_replacement = '''        MapWaypointView Waypoint;
        Waypoint.Active = WaypointActive;
        Waypoint.Position = WaypointPosition;
        Waypoint.DistanceMeters = CurrentWaypointDistance();

        const float ThreatDistance =
            Hunter.IsActive()
                ? glm::distance(
                    glm::vec2{GamePlayer.Position().x, GamePlayer.Position().z},
                    glm::vec2{Hunter.Position().x, Hunter.Position().z}
                )
                : std::numeric_limits<float>::infinity();

        GameRenderer.DrawMiniMapV2(
            World,
            {GamePlayer.Position().x, GamePlayer.Position().z},
            {GamePlayer.Forward().x, GamePlayer.Forward().z},
            BuildMapMarkers(),
            Waypoint,
            WaypointPath,
            ThreatDistance,
            Time
        );

        if (GamePlayer.TouchInputActive())
            GameRenderer.DrawTouchControlsV1();'''
game_cpp, count = mini_pattern.subn(mini_replacement, game_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace minimap render")
write("src/Game/Game.cpp", game_cpp)

cmake = read("CMakeLists.txt").replace(
    "project(BackroomsOffical VERSION 0.3.26 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.27 LANGUAGES C CXX)",
    1,
)
write("CMakeLists.txt", cmake)

version = read("src/Core/Version.h").replace('Text = "0.3.26"', 'Text = "0.3.27"', 1)
write("src/Core/Version.h", version)

resource = read("src/Platform/Windows/Backrooms.rc")
resource = resource.replace("FILEVERSION 0,3,26,0", "FILEVERSION 0,3,27,0", 1)
resource = resource.replace("PRODUCTVERSION 0,3,26,0", "PRODUCTVERSION 0,3,27,0", 1)
resource = resource.replace('"0.3.26\\0"', '"0.3.27\\0"')
write("src/Platform/Windows/Backrooms.rc", resource)

notes = (
    "V0.3.27 replaces the debug-looking V0.3.26 map with a polished infinite Level 0 atlas: "
    "viewport-sized deterministic map generation follows the camera forever in every direction with no hard map bounds, "
    "a visible Back button and ESC-to-menu guidance return to the pause menu, hover labels identify map icons, the player uses a directional arrow, "
    "breakers use red neon ring/screwdriver markers with green online state, full-map click or touch creates a waypoint, A* routing follows maze openings, "
    "G/random-destination creates exploration targets, waypoint distance appears on the map and minimap and clears automatically when reached, "
    "nearby monsters make the minimap flash red/black, F11 toggles fullscreen, and SDL3 gamepad plus touch controls add responsive device input and mobile-scaled HUD controls."
)
write("update/release_notes.txt", notes + "\n")

print("V0.3.27 source pass applied")
