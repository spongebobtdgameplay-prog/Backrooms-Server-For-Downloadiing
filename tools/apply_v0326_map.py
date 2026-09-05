from pathlib import Path
import re


def Read(PathName):
    return Path(PathName).read_text(encoding="utf-8")


def Write(PathName, Content):
    Path(PathName).write_text(Content, encoding="utf-8", newline="\n")


def ReplaceOnce(Content, Old, New, Label):
    if Old not in Content:
        raise RuntimeError(f"Missing patch marker: {Label}")
    return Content.replace(Old, New, 1)


RendererHeader = Read("src/Rendering/Renderer.h")
RendererHeader = ReplaceOnce(
    RendererHeader,
    "    Resume,\n    MainMenu\n};",
    "    Resume,\n    Map,\n    MainMenu\n};\n\nenum class MapMarkerKind\n{\n    Breaker,\n    BreakerActive,\n    Exit,\n    ExitPowered,\n    Entity\n};\n\nstruct MapMarker\n{\n    glm::vec2 Position{0.0f};\n    MapMarkerKind Kind = MapMarkerKind::Breaker;\n};",
    "map marker types",
)
RendererHeader = ReplaceOnce(
    RendererHeader,
    "    void DrawGameplayOverlayV3(\n        float Stamina,\n        int BreakersActive,\n        int BreakersRequired,\n        int InteractionType,\n        bool CanExit,\n        float Fps\n    );",
    "    void DrawGameplayOverlayV3(\n        float Stamina,\n        int BreakersActive,\n        int BreakersRequired,\n        int InteractionType,\n        bool CanExit,\n        float Fps\n    );\n\n    void DrawMiniMapV1(\n        const WorldData& World,\n        const glm::vec2& PlayerPosition,\n        const glm::vec2& PlayerForward,\n        const std::vector<MapMarker>& Markers\n    );\n\n    void DrawFullMapV1(\n        const WorldData& World,\n        const glm::vec2& MapCenter,\n        float Zoom,\n        const glm::vec2& PlayerPosition,\n        const glm::vec2& PlayerForward,\n        const std::vector<MapMarker>& Markers,\n        int BreakersActive,\n        int BreakersRequired\n    );",
    "map draw declarations",
)
RendererHeader = ReplaceOnce(
    RendererHeader,
    "    int TextWidth(const std::string& Text, int Scale);\n    void DrawRect(",
    "    int TextWidth(const std::string& Text, int Scale);\n    void DrawMapPanel(\n        const WorldData& World,\n        int X,\n        int Y,\n        int PanelWidth,\n        int PanelHeight,\n        const glm::vec2& MapCenter,\n        float PixelsPerMeter,\n        const glm::vec2& PlayerPosition,\n        const glm::vec2& PlayerForward,\n        const std::vector<MapMarker>& Markers,\n        bool Detailed\n    );\n    void DrawRect(",
    "private map panel declaration",
)
RendererHeader = ReplaceOnce(
    RendererHeader,
    "    UiRect PauseResumeRect;\n    UiRect PauseMainMenuRect;",
    "    UiRect PauseMapRect;\n    UiRect PauseResumeRect;\n    UiRect PauseMainMenuRect;",
    "pause map rect",
)
RendererHeader = ReplaceOnce(
    RendererHeader,
    "    float PauseResumeHover = 0.0f;\n    float PauseMainMenuHover = 0.0f;",
    "    float PauseMapHover = 0.0f;\n    float PauseResumeHover = 0.0f;\n    float PauseMainMenuHover = 0.0f;",
    "pause map hover",
)
Write("src/Rendering/Renderer.h", RendererHeader)


GameHeader = Read("src/Game/Game.h")
GameHeader = re.sub(
    r"    bool HasMenuOverlay\(\) const\n    \{.*?    \}\n\n    void RenderMenuOverlay\(\)\n    \{.*?    \}\n",
    "    bool HasMenuOverlay() const;\n    void RenderMenuOverlay();\n",
    GameHeader,
    count=1,
    flags=re.S,
)
GameHeader = ReplaceOnce(
    GameHeader,
    "    void Reset();\n    glm::vec3 MenuPointerFromEvent",
    "    void Reset();\n    std::vector<MapMarker> BuildMapMarkers() const;\n    void OpenMap(bool ReturnToPause);\n    void CloseMap();\n    glm::vec3 MenuPointerFromEvent",
    "game map private methods",
)
GameHeader = ReplaceOnce(
    GameHeader,
    "    bool InteractPressed = false;\n    bool RestartPressed = false;\n\n    int FrameCounter = 0;",
    "    bool InteractPressed = false;\n    bool RestartPressed = false;\n\n    bool MapOpen = false;\n    bool MapReturnToPause = false;\n    bool MapDragging = false;\n    glm::vec2 MapCenter{0.0f};\n    glm::vec2 MapDragPointer{0.0f};\n    float MapZoom = 1.0f;\n\n    int FrameCounter = 0;",
    "game map state",
)
Write("src/Game/Game.h", GameHeader)


WorldHeader = Read("src/World/WorldGenerator.h")
WorldHeader = ReplaceOnce(
    WorldHeader,
    "    WorldData Build();\n    WorldData BuildAround(const glm::vec3& FocusPosition);",
    "    WorldData Build();\n    WorldData BuildAround(const glm::vec3& FocusPosition);\n    WorldData BuildMapAround(const glm::vec3& FocusPosition);",
    "BuildMapAround declaration",
)
Write("src/World/WorldGenerator.h", WorldHeader)


WorldSource = Read("src/World/WorldGenerator.cpp")
MapBuildMethod = r'''
WorldData WorldGenerator::BuildMapAround(
    const glm::vec3& FocusPosition
)
{
    WorldData World;

    World.ChunkCells = ChunkCellCount;
    World.StreamRadius = ActiveChunkRadius;
    World.Columns =
        ChunkCellCount * (ActiveChunkRadius * 2 + 1);
    World.Rows = World.Columns;
    World.CellSize = DefaultCellSize;

    World.CenterChunkX = ChunkCoordinate(FocusPosition.x);
    World.CenterChunkZ = ChunkCoordinate(FocusPosition.z);

    const int FirstChunkX =
        World.CenterChunkX - ActiveChunkRadius;

    const int FirstChunkZ =
        World.CenterChunkZ - ActiveChunkRadius;

    World.OriginCellX =
        FirstChunkX * ChunkCellCount -
        ChunkHalfCells;

    World.OriginCellZ =
        FirstChunkZ * ChunkCellCount -
        ChunkHalfCells;

    CreateStreamedMaze(World);
    return World;
}

'''
WorldSource = ReplaceOnce(
    WorldSource,
    "bool WorldGenerator::NeedsRebuild(\n",
    MapBuildMethod + "bool WorldGenerator::NeedsRebuild(\n",
    "BuildMapAround implementation",
)
Write("src/World/WorldGenerator.cpp", WorldSource)


MapOverlay = r'''#include "Renderer.h"

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
'''
Write("src/Rendering/MapOverlay.cpp", MapOverlay)


GameSource = Read("src/Game/Game.cpp")
GameMethods = r'''
bool Game::HasMenuOverlay() const
{
    return State.MainMenuOpen || State.Paused || MapOpen;
}

std::vector<MapMarker> Game::BuildMapMarkers() const
{
    std::vector<MapMarker> Markers;
    Markers.reserve(Breakers.size() + 2);

    for (const Breaker& BreakerData : Breakers)
    {
        Markers.push_back({
            {BreakerData.Position.x, BreakerData.Position.z},
            BreakerData.Active
                ? MapMarkerKind::BreakerActive
                : MapMarkerKind::Breaker
        });
    }

    Markers.push_back({
        {ExitPosition.x, ExitPosition.z},
        State.CanExit()
            ? MapMarkerKind::ExitPowered
            : MapMarkerKind::Exit
    });

    if (Hunter.IsActive())
    {
        Markers.push_back({
            {Hunter.Position().x, Hunter.Position().z},
            MapMarkerKind::Entity
        });
    }

    return Markers;
}

void Game::OpenMap(bool ReturnToPause)
{
    MapOpen = true;
    MapReturnToPause = ReturnToPause;
    MapDragging = false;
    MapZoom = std::clamp(MapZoom, 0.65f, 2.8f);
    MapCenter = {
        GamePlayer.Position().x,
        GamePlayer.Position().z
    };

    State.Paused = false;
    State.MainMenuOpen = false;
    GameRenderer.ClearMenuPointer();
}

void Game::CloseMap()
{
    const bool ReturnToPause = MapReturnToPause;

    MapOpen = false;
    MapReturnToPause = false;
    MapDragging = false;
    GameRenderer.ClearMenuPointer();

    if (ReturnToPause)
        State.Paused = true;
}

void Game::RenderMenuOverlay()
{
    GameRenderer.BeginFrame();

    if (MapOpen)
    {
        WorldGenerator Generator(Seed);
        const WorldData MapWorld =
            Generator.BuildMapAround({MapCenter.x, 0.0f, MapCenter.y});

        GameRenderer.DrawFullMapV1(
            MapWorld,
            MapCenter,
            MapZoom,
            {GamePlayer.Position().x, GamePlayer.Position().z},
            {GamePlayer.Forward().x, GamePlayer.Forward().z},
            BuildMapMarkers(),
            State.BreakersActive,
            State.BreakersRequired
        );
        return;
    }

    if (State.Paused)
        GameRenderer.DrawPauseMenuV3();
    else
        GameRenderer.DrawMainMenuV3(State.Started);
}

'''
GameSource = ReplaceOnce(
    GameSource,
    "glm::vec3 Game::MenuPointerFromEvent(const SDL_Event& Event) const\n",
    GameMethods + "glm::vec3 Game::MenuPointerFromEvent(const SDL_Event& Event) const\n",
    "game map methods",
)
GameSource = ReplaceOnce(
    GameSource,
    "    InteractPressed = false;\n    RestartPressed = false;\n\n    FrameCounter = 0;",
    "    InteractPressed = false;\n    RestartPressed = false;\n\n    MapOpen = false;\n    MapReturnToPause = false;\n    MapDragging = false;\n    MapCenter = {0.0f, 0.0f};\n    MapDragPointer = {0.0f, 0.0f};\n    MapZoom = 1.0f;\n\n    FrameCounter = 0;",
    "reset map state",
)
MapEventBlock = r'''
    if (MapOpen)
    {
        if (
            KeyDown &&
            (
                Event.key.scancode == SDL_SCANCODE_M ||
                Event.key.scancode == SDL_SCANCODE_ESCAPE
            )
        )
        {
            CloseMap();
            return;
        }

        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_R
        )
        {
            MapCenter = {
                GamePlayer.Position().x,
                GamePlayer.Position().z
            };
            return;
        }

        if (Event.type == SDL_EVENT_MOUSE_WHEEL)
        {
            const float Factor =
                Event.wheel.y > 0.0f ? 1.12f : 0.89f;

            MapZoom = std::clamp(
                MapZoom * Factor,
                0.65f,
                2.8f
            );
            return;
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            Event.button.button == SDL_BUTTON_LEFT
        )
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            MapDragging = true;
            MapDragPointer = {Pointer.x, Pointer.y};
            return;
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
            Event.button.button == SDL_BUTTON_LEFT
        )
        {
            MapDragging = false;
            return;
        }

        if (
            Event.type == SDL_EVENT_MOUSE_MOTION &&
            MapDragging
        )
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            const glm::vec2 CurrentPointer{Pointer.x, Pointer.y};
            const glm::vec2 Delta = CurrentPointer - MapDragPointer;
            const float PixelsPerMeter =
                8.0f * std::max(MapZoom, 0.65f);

            MapCenter.x -= Delta.x / PixelsPerMeter;
            MapCenter.y -= Delta.y / PixelsPerMeter;
            MapDragPointer = CurrentPointer;
            return;
        }

        return;
    }

'''
GameSource = ReplaceOnce(
    GameSource,
    "    if (State.MainMenuOpen)\n",
    MapEventBlock + "    if (State.MainMenuOpen)\n",
    "map input block",
)
GameSource = ReplaceOnce(
    GameSource,
    "            if (Action == MenuUiAction::Resume)\n",
    "            if (Action == MenuUiAction::Map)\n            {\n                OpenMap(true);\n                return;\n            }\n\n            if (Action == MenuUiAction::Resume)\n",
    "pause map click action",
)
GameSource = re.sub(
    r"        if \(\n            KeyDown &&\n            Event\.key\.scancode == SDL_SCANCODE_M\n        \)\n        \{\n            State\.Paused = false;\n            State\.MainMenuOpen = true;\n            GameRenderer\.ClearMenuPointer\(\);\n            return;\n        \}",
    "        if (\n            KeyDown &&\n            Event.key.scancode == SDL_SCANCODE_M\n        )\n        {\n            OpenMap(true);\n            return;\n        }",
    GameSource,
    count=1,
)
GameSource = re.sub(
    r"    if \(\n        State\.Started &&\n        !State\.Ended &&\n        KeyDown &&\n        Event\.key\.scancode == SDL_SCANCODE_M\n    \)\n    \{\n        State\.MainMenuOpen = true;\n        State\.Paused = false;\n        GameRenderer\.ClearMenuPointer\(\);\n        return;\n    \}",
    "    if (\n        State.Started &&\n        !State.Ended &&\n        KeyDown &&\n        Event.key.scancode == SDL_SCANCODE_M\n    )\n    {\n        OpenMap(false);\n        return;\n    }",
    GameSource,
    count=1,
)
GameSource = ReplaceOnce(
    GameSource,
    "        !State.MainMenuOpen &&\n        !State.Paused &&\n        !State.Ended;",
    "        !State.MainMenuOpen &&\n        !State.Paused &&\n        !MapOpen &&\n        !State.Ended;",
    "mouse capture excludes map",
)
MapUpdateBlock = r'''
    if (MapOpen)
    {
        int KeyCount = 0;
        const bool* Keys = SDL_GetKeyboardState(&KeyCount);
        glm::vec2 Pan{0.0f};

        auto KeyHeld = [&](SDL_Scancode ScanCode)
        {
            const int Index = static_cast<int>(ScanCode);
            return
                Keys != nullptr &&
                Index >= 0 &&
                Index < KeyCount &&
                Keys[Index];
        };

        if (KeyHeld(SDL_SCANCODE_W) || KeyHeld(SDL_SCANCODE_UP))
            Pan.y -= 1.0f;
        if (KeyHeld(SDL_SCANCODE_S) || KeyHeld(SDL_SCANCODE_DOWN))
            Pan.y += 1.0f;
        if (KeyHeld(SDL_SCANCODE_A) || KeyHeld(SDL_SCANCODE_LEFT))
            Pan.x -= 1.0f;
        if (KeyHeld(SDL_SCANCODE_D) || KeyHeld(SDL_SCANCODE_RIGHT))
            Pan.x += 1.0f;

        if (glm::length(Pan) > 0.001f)
        {
            Pan = glm::normalize(Pan);
            const float PanSpeed =
                72.0f / std::max(MapZoom, 0.65f);
            MapCenter += Pan * PanSpeed * DeltaTime;
        }

        InteractPressed = false;
        RestartPressed = false;
        return;
    }

'''
GameSource = ReplaceOnce(
    GameSource,
    "    if (\n        !State.Started ||\n        State.MainMenuOpen ||\n        State.Paused\n    )",
    MapUpdateBlock + "    if (\n        !State.Started ||\n        State.MainMenuOpen ||\n        State.Paused\n    )",
    "map continuous pan",
)
GameSource = ReplaceOnce(
    GameSource,
    "        GameRenderer.DrawGameplayOverlayV3(\n            GamePlayer.Stamina(),\n            State.BreakersActive,\n            State.BreakersRequired,\n            InteractionType,\n            State.CanExit(),\n            DisplayedFps\n        );",
    "        GameRenderer.DrawGameplayOverlayV3(\n            GamePlayer.Stamina(),\n            State.BreakersActive,\n            State.BreakersRequired,\n            InteractionType,\n            State.CanExit(),\n            DisplayedFps\n        );\n\n        GameRenderer.DrawMiniMapV1(\n            World,\n            {GamePlayer.Position().x, GamePlayer.Position().z},\n            {GamePlayer.Forward().x, GamePlayer.Forward().z},\n            BuildMapMarkers()\n        );",
    "gameplay minimap draw",
)
Write("src/Game/Game.cpp", GameSource)


MenuSource = Read("src/Rendering/InteractiveMenu.cpp")
MenuSource = ReplaceOnce(
    MenuSource,
    "    MoveToward(\n        PauseResumeHover,\n        PauseResumeRect.Contains(MenuPointerX, MenuPointerY)\n    );",
    "    MoveToward(\n        PauseMapHover,\n        PauseMapRect.Contains(MenuPointerX, MenuPointerY)\n    );\n\n    MoveToward(\n        PauseResumeHover,\n        PauseResumeRect.Contains(MenuPointerX, MenuPointerY)\n    );",
    "pause map hover update",
)
MenuSource = ReplaceOnce(
    MenuSource,
    "MenuUiAction Renderer::HitTestPauseMenu() const\n{\n    if (PauseResumeRect.Contains(MenuPointerX, MenuPointerY))",
    "MenuUiAction Renderer::HitTestPauseMenu() const\n{\n    if (PauseMapRect.Contains(MenuPointerX, MenuPointerY))\n        return MenuUiAction::Map;\n\n    if (PauseResumeRect.Contains(MenuPointerX, MenuPointerY))",
    "pause map hit test",
)
MenuSource = MenuSource.replace(
    '"ESC PAUSE   M MAIN MENU"',
    '"ESC PAUSE   M MAP"',
)
PauseFunction = r'''void Renderer::DrawPauseMenuV3()
{
    glDisable(GL_DEPTH_TEST);
    DrawMenuBackdrop();

    const glm::vec3 Yellow{200.0f / 255.0f, 187.0f / 255.0f, 97.0f / 255.0f};
    const glm::vec3 Ink{39.0f / 255.0f, 35.0f / 255.0f, 15.0f / 255.0f};
    const glm::vec3 Muted{0.34f, 0.31f, 0.15f};

    const int Margin =
        std::clamp(static_cast<int>(Width) / 24, 28, 66);

    DrawMenuText(
        "BACKROOMS OFFICAL",
        Margin,
        30,
        13,
        700,
        0.18f,
        Muted,
        1.0f,
        false
    );

    DrawMenuText(
        "PAUSE",
        Margin,
        64,
        std::clamp(static_cast<int>(Width) / 18, 54, 88),
        900,
        -0.045f,
        Ink,
        1.0f,
        false
    );

    const int NavY = 164;
    const int NavGap = 12;
    const int Available =
        std::max(static_cast<int>(Width) - Margin * 2 - NavGap * 2, 3);
    const int NavWidth =
        std::clamp(Available / 3, 150, 300);
    const int NavHeight = 52;

    PauseMapRect = {Margin, NavY, NavWidth, NavHeight};
    PauseResumeRect = {Margin + NavWidth + NavGap, NavY, NavWidth, NavHeight};
    PauseMainMenuRect = {Margin + (NavWidth + NavGap) * 2, NavY, NavWidth, NavHeight};

    auto DrawTab = [&](const UiRect& Rect, const std::string& Label, float Hover)
    {
        const float Amount = std::clamp(Hover, 0.0f, 1.0f);

        if (Amount > 0.001f)
        {
            const glm::vec3 Fill = Yellow * (1.0f - Amount * 0.12f) + Ink * (Amount * 0.12f);
            DrawRect(Rect.X, Rect.Y + 2, Rect.Width, Rect.Height - 4, Fill);
        }

        DrawRect(Rect.X, Rect.Y, Rect.Width, 2, Ink);
        DrawRect(Rect.X, Rect.Y + Rect.Height - 2, Rect.Width, 2, Ink);

        const int Padding = 14 + static_cast<int>(std::round(Amount * 12.0f));

        DrawMenuText(
            Label,
            Rect.X + Padding,
            Rect.Y + 17,
            14,
            800,
            0.12f,
            Ink,
            1.0f,
            false
        );
    };

    DrawTab(PauseMapRect, "MAP", PauseMapHover);
    DrawTab(PauseResumeRect, "RESUME", PauseResumeHover);
    DrawTab(PauseMainMenuRect, "MAIN MENU", PauseMainMenuHover);

    const int DetailY = NavY + NavHeight + 56;

    DrawMenuText(
        "LEVEL 0",
        Margin,
        DetailY,
        12,
        700,
        0.20f,
        Muted,
        1.0f,
        false
    );

    DrawMenuText(
        "UNSTABLE SESSION",
        Margin,
        DetailY + 30,
        30,
        900,
        -0.02f,
        Ink,
        1.0f,
        false
    );

    DrawMenuText(
        "MAP OPENS THE INFINITE LEVEL 0 GRID. PAN FOREVER, TRACK BREAKERS, EXIT AND THREATS.",
        Margin,
        DetailY + 86,
        13,
        650,
        0.03f,
        Muted,
        1.0f,
        false
    );

    DrawRect(
        Margin,
        DetailY + 128,
        std::max(static_cast<int>(Width) - Margin * 2, 1),
        1,
        Muted
    );

    DrawMenuText(
        "M MAP     ESC RESUME",
        Margin,
        static_cast<int>(Height) - 46,
        10,
        700,
        0.15f,
        Muted,
        1.0f,
        false
    );

    glEnable(GL_DEPTH_TEST);
}

'''
MenuSource, Replaced = re.subn(
    r"void Renderer::DrawPauseMenuV3\(\)\n\{.*?\n\}\n\nvoid Renderer::DrawGameplayOverlayV3",
    PauseFunction + "void Renderer::DrawGameplayOverlayV3",
    MenuSource,
    count=1,
    flags=re.S,
)
if Replaced != 1:
    raise RuntimeError("Could not replace pause menu V3")
MenuSource = ReplaceOnce(
    MenuSource,
    "    const int SprintY =\n        static_cast<int>(Height) - 38;",
    "    const int SprintY =\n        std::max(80, static_cast<int>(Height) - 226);",
    "sprint above minimap",
)
Write("src/Rendering/InteractiveMenu.cpp", MenuSource)


CMake = Read("CMakeLists.txt")
CMake = CMake.replace(
    "project(BackroomsOffical VERSION 0.3.25 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.26 LANGUAGES C CXX)",
)
Write("CMakeLists.txt", CMake)

Version = Read("src/Core/Version.h").replace('Text = "0.3.25"', 'Text = "0.3.26"')
Write("src/Core/Version.h", Version)

Rc = Read("src/Platform/Windows/Backrooms.rc")
Rc = Rc.replace("0,3,25,0", "0,3,26,0").replace("0.3.25", "0.3.26")
Write("src/Platform/Windows/Backrooms.rc", Rc)

Write(
    "update/release_notes.txt",
    "V0.3.26 adds a GTA-style native map system for the infinite Level 0 world: a yellow wallpaper-textured corner minimap during gameplay, breaker/activated-breaker/exit/powered-exit/entity markers, a redesigned GTA-inspired pause menu with a MAP tab, M to open the full map directly, mouse drag plus WASD/arrow panning with no coordinate bounds, mouse-wheel zoom, R recenter, and deterministic map-only chunk generation so the full map can keep scrolling through the same infinite maze topology without loading full 3D environment geometry. The sprint HUD is moved above the minimap so the two systems do not overlap."
)

Required = [
    "MapMarkerKind",
    "DrawMiniMapV1",
    "DrawFullMapV1",
    "BuildMapAround",
    "MapOpen",
    "OpenMap(false)",
    "MenuUiAction::Map",
    "M  MAP",
]
Combined = "\n".join([
    Read("src/Rendering/Renderer.h"),
    Read("src/Rendering/MapOverlay.cpp"),
    Read("src/Game/Game.cpp"),
    Read("src/Rendering/InteractiveMenu.cpp"),
    Read("src/World/WorldGenerator.cpp"),
])
for Marker in Required:
    if Marker not in Combined:
        raise RuntimeError(f"Missing V0.3.26 marker: {Marker}")
