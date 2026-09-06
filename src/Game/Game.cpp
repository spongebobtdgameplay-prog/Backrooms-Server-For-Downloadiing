#include "Game.h"
#include "MapNavigation.h"

#include "../Physics/Raycast.h"
#include "../Updater/UpdaterService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <utility>

bool Game::Initialize(uint32_t NewWidth, uint32_t NewHeight)
{
    Width = NewWidth;
    Height = NewHeight;

    if (!GameRenderer.Initialize())
        return false;

    GameRenderer.Resize(Width, Height);
    Audio.Initialize();

    Reset();

    return true;
}

void Game::Shutdown()
{
    GamePlayer.ShutdownInput();
    Audio.Shutdown();
    GameRenderer.ShutdownInterfaceV3();
    GameRenderer.Shutdown();
}

void Game::Resize(uint32_t NewWidth, uint32_t NewHeight)
{
    Width = std::max(NewWidth, 1u);
    Height = std::max(NewHeight, 1u);

    GameRenderer.Resize(Width, Height);
}


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
    const float ChunkMeters = std::max(
        World.CellSize * static_cast<float>(World.ChunkCells),
        1.0f
    );
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

    const float CellSize = std::max(World.CellSize, 1.0f);
    WaypointRouteCellX = static_cast<int>(std::round(Start.x / CellSize));
    WaypointRouteCellZ = static_cast<int>(std::round(Start.y / CellSize));
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

void Game::RenderMenuOverlay()
{
    GameRenderer.BeginFrame();

    if (MapOpen)
    {
        const float Scale = 7.2f * std::clamp(MapZoom, 0.45f, 4.0f);
        const float HalfWidthMeters = static_cast<float>(Width) * 0.5f / Scale;
        const float HalfHeightMeters = static_cast<float>(Height) * 0.5f / Scale;
        const float ChunkMeters = std::max(
            World.CellSize * static_cast<float>(World.ChunkCells),
            1.0f
        );
        const int Radius = std::clamp(
            static_cast<int>(std::ceil(
                std::max(HalfWidthMeters, HalfHeightMeters) / ChunkMeters
            )) + 2,
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

        const float Time =
            static_cast<float>(SDL_GetTicksNS()) /
            1000000000.0f;

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

glm::vec3 Game::MenuPointerFromEvent(const SDL_Event& Event) const
{
    float X = -10000.0f;
    float Y = -10000.0f;
    SDL_WindowID WindowId = 0;

    if (Event.type == SDL_EVENT_MOUSE_MOTION)
    {
        X = Event.motion.x;
        Y = Event.motion.y;
        WindowId = Event.motion.windowID;
    }
    else if (
        Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        Event.type == SDL_EVENT_MOUSE_BUTTON_UP
    )
    {
        X = Event.button.x;
        Y = Event.button.y;
        WindowId = Event.button.windowID;
    }
    else
    {
        return {X, Y, 0.0f};
    }

    SDL_Window* EventWindow = SDL_GetWindowFromID(WindowId);

    int LogicalWidth = static_cast<int>(Width);
    int LogicalHeight = static_cast<int>(Height);

    if (EventWindow != nullptr)
    {
        int WindowWidth = 0;
        int WindowHeight = 0;

        if (
            SDL_GetWindowSize(
                EventWindow,
                &WindowWidth,
                &WindowHeight
            ) &&
            WindowWidth > 0 &&
            WindowHeight > 0
        )
        {
            LogicalWidth = WindowWidth;
            LogicalHeight = WindowHeight;
        }
    }

    const float ScaleX =
        static_cast<float>(Width) /
        static_cast<float>(std::max(LogicalWidth, 1));

    const float ScaleY =
        static_cast<float>(Height) /
        static_cast<float>(std::max(LogicalHeight, 1));

    return {
        X * ScaleX,
        Y * ScaleY,
        0.0f
    };
}

bool Game::TryMountBreaker(
    const glm::vec3& CellCenter,
    int Variant,
    Breaker& Result
) const
{
    const int WorldX =
        static_cast<int>(
            std::round(CellCenter.x / World.CellSize)
        );

    const int WorldZ =
        static_cast<int>(
            std::round(CellCenter.z / World.CellSize)
        );

    const int X = WorldX - World.OriginCellX;
    const int Z = WorldZ - World.OriginCellZ;

    if (
        X < 0 ||
        Z < 0 ||
        X >= World.Columns ||
        Z >= World.Rows
    )
    {
        return false;
    }

    const MazeCell& Cell = World.Cell(X, Z);

    const int StartDirection =
        static_cast<int>(
            (Seed + static_cast<uint32_t>(Variant * 2654435761u)) % 4u
        );

    for (int Offset = 0; Offset < 4; ++Offset)
    {
        const int DirectionIndex =
            (StartDirection + Offset) % 4;

        if (!Cell.Walls[static_cast<std::size_t>(DirectionIndex)])
            continue;

        glm::vec3 WallCenter = CellCenter;
        glm::vec3 Forward{0.0f};

        if (DirectionIndex == 0)
        {
            WallCenter.z -= World.CellSize * 0.5f;
            Forward = {0.0f, 0.0f, 1.0f};
        }
        else if (DirectionIndex == 1)
        {
            WallCenter.x += World.CellSize * 0.5f;
            Forward = {-1.0f, 0.0f, 0.0f};
        }
        else if (DirectionIndex == 2)
        {
            WallCenter.z += World.CellSize * 0.5f;
            Forward = {0.0f, 0.0f, -1.0f};
        }
        else
        {
            WallCenter.x -= World.CellSize * 0.5f;
            Forward = {1.0f, 0.0f, 0.0f};
        }

        const glm::vec3 Right{
            Forward.z,
            0.0f,
            -Forward.x
        };

        const uint32_t Hash =
            Seed ^
            static_cast<uint32_t>(Variant * 2246822519u) ^
            static_cast<uint32_t>((WorldX + 4097) * 3266489917u) ^
            static_cast<uint32_t>((WorldZ + 8191) * 668265263u);

        const float Along =
            (static_cast<float>(Hash % 1000u) / 999.0f - 0.5f) *
            1.45f;

        Result.Position =
            WallCenter +
            Forward * (World.WallThickness * 0.5f + 0.36f) +
            Right * Along;

        Result.Position.y = 0.88f;
        Result.Forward = Forward;
        Result.Active = false;
        return true;
    }

    return false;
}

void Game::Reset()
{
    const auto Now =
        std::chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count();

    Seed = static_cast<uint32_t>(Now);

    WorldGenerator Generator(Seed);
    World = Generator.BuildAround({0.0f, 0.0f, 0.0f});

    GamePlayer.Reset({0.0f, 1.65f, 0.0f});
    PreviousPlayerPosition = GamePlayer.Position();
    FootstepDistance = 0.0f;

    State = {};
    Breakers.clear();

    std::unordered_set<int> Used;
    Used.insert(0);

    auto Pick = [&](float MinDistance)
    {
        for (int Attempts = 0; Attempts < 400; ++Attempts)
        {
            const uint32_t Hash =
                Seed * 1664525u +
                static_cast<uint32_t>(Attempts * 1013904223u) +
                static_cast<uint32_t>(Used.size() * 747796405u);

            const int Index =
                static_cast<int>(
                    Hash %
                    static_cast<uint32_t>(World.OpenCells.size())
                );

            if (Used.contains(Index))
                continue;

            const glm::vec3 Position =
                World.OpenCells[static_cast<std::size_t>(Index)];

            if (
                glm::distance(
                    glm::vec2(Position.x, Position.z),
                    glm::vec2(0.0f, 0.0f)
                ) < MinDistance
            )
            {
                continue;
            }

            Used.insert(Index);
            return Position;
        }

        return World.OpenCells.back();
    };

    for (int I = 0; I < 3; ++I)
    {
        Breaker Mounted;
        bool MountedSuccessfully = false;

        for (int Attempt = 0; Attempt < 36; ++Attempt)
        {
            const glm::vec3 Candidate = Pick(24.0f);

            if (TryMountBreaker(
                    Candidate,
                    I * 41 + Attempt,
                    Mounted
                ))
            {
                MountedSuccessfully = true;
                break;
            }
        }

        if (!MountedSuccessfully)
            continue;

        Breakers.push_back(Mounted);
        World.Colliders.push_back(
            BreakerBounds(Breakers.back())
        );
    }

    Breaker MountedExit;
    bool ExitMounted = false;

    for (int Attempt = 0; Attempt < 48; ++Attempt)
    {
        const glm::vec3 Candidate = Pick(38.0f);

        if (TryMountBreaker(
                Candidate,
                900 + Attempt,
                MountedExit
            ))
        {
            ExitMounted = true;
            break;
        }
    }

    if (ExitMounted)
    {
        ExitPosition = MountedExit.Position;
        ExitPosition.y = 0.0f;
        ExitForward = MountedExit.Forward;
    }
    else
    {
        ExitPosition = Pick(38.0f);
        ExitPosition.y = 0.0f;
        ExitForward = {0.0f, 0.0f, 1.0f};
    }

    World.Colliders.push_back(ExitBounds());

    const glm::vec3 EntityPosition = Pick(50.0f);
    Hunter.Reset(EntityPosition);

    InteractionType = 0;
    InteractionIndex = -1;

    InteractPressed = false;
    RestartPressed = false;

    MapOpen = false;
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

    FrameCounter = 0;
    FpsCounterStart = 0;
    DisplayedFps = 0.0f;

    Message.clear();
    MessageTimer = 0.0f;

    GameRenderer.ClearMenuPointer();

    UpdateTitle();
}

void Game::HandleEvent(
    const SDL_Event& Event,
    bool MouseCaptured
)
{
    const bool KeyDown =
        Event.type == SDL_EVENT_KEY_DOWN &&
        !Event.key.repeat;

    const bool Activate =
        KeyDown &&
        (
            Event.key.scancode == SDL_SCANCODE_RETURN ||
            Event.key.scancode == SDL_SCANCODE_SPACE
        );

    const bool PointerEvent =
        Event.type == SDL_EVENT_MOUSE_MOTION ||
        Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        Event.type == SDL_EVENT_MOUSE_BUTTON_UP;


    if (MapOpen)
    {
        if (PointerEvent)
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
        }

        if (KeyDown && Event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            CloseMap(true);
            return;
        }

        if (KeyDown && Event.key.scancode == SDL_SCANCODE_M)
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
            SetWaypoint(
                GameRenderer.FullMapPointerToWorld(MapCenter, MapZoom)
            );
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
                SetWaypoint(
                    GameRenderer.FullMapPointerToWorld(MapCenter, MapZoom)
                );
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
            const float PixelsPerMeter =
                7.2f * std::max(MapZoom, 0.45f);

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
            const float PixelsPerMeter =
                7.2f * std::max(MapZoom, 0.45f);

            MapCenter.x -= Delta.x / PixelsPerMeter;
            MapCenter.y -= Delta.y / PixelsPerMeter;
            MapDragDistance += glm::length(Delta);
            MapDragPointer = Pointer;
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
            return;
        }

        if (
            (
                Event.type == SDL_EVENT_FINGER_UP ||
                Event.type == SDL_EVENT_FINGER_CANCELED
            ) &&
            MapTouchDragging &&
            Event.tfinger.fingerID == MapTouchFinger
        )
        {
            const glm::vec2 Pointer{
                std::clamp(Event.tfinger.x, 0.0f, 1.0f) * static_cast<float>(Width),
                std::clamp(Event.tfinger.y, 0.0f, 1.0f) * static_cast<float>(Height)
            };
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);

            if (
                MapDragDistance <= 10.0f &&
                GameRenderer.PointerInsideFullMap()
            )
            {
                SetWaypoint(
                    GameRenderer.FullMapPointerToWorld(MapCenter, MapZoom)
                );
            }

            MapTouchDragging = false;
            MapTouchFinger = 0;
            MapDragDistance = 0.0f;
            return;
        }

        return;
    }

    if (State.MainMenuOpen)
    {
        if (PointerEvent)
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            Event.button.button == SDL_BUTTON_LEFT
        )
        {
            const MenuUiAction Action =
                GameRenderer.HitTestMainMenu(State.Started);

            if (Action == MenuUiAction::Primary)
            {
                State.Started = true;
                State.MainMenuOpen = false;
                State.Paused = false;
                GameRenderer.ClearMenuPointer();
                return;
            }

            if (
                Action == MenuUiAction::NewSession &&
                State.Started
            )
            {
                Reset();
                State.Started = true;
                State.MainMenuOpen = false;
                State.Paused = false;
                GameRenderer.ClearMenuPointer();
                return;
            }
        }

        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_N &&
            State.Started
        )
        {
            Reset();
            State.Started = true;
            State.MainMenuOpen = false;
            State.Paused = false;
            GameRenderer.ClearMenuPointer();
            return;
        }

        if (
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

        return;
    }

    if (State.Paused)
    {
        if (PointerEvent)
        {
            const glm::vec3 Pointer = MenuPointerFromEvent(Event);
            GameRenderer.SetMenuPointer(Pointer.x, Pointer.y);
        }

        if (
            Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            Event.button.button == SDL_BUTTON_LEFT
        )
        {
            const MenuUiAction Action =
                GameRenderer.HitTestPauseMenu();

            if (Action == MenuUiAction::Map)
            {
                OpenMap(true);
                return;
            }

            if (Action == MenuUiAction::Resume)
            {
                State.Paused = false;
                GameRenderer.ClearMenuPointer();
                return;
            }

            if (Action == MenuUiAction::MainMenu)
            {
                State.Paused = false;
                State.MainMenuOpen = true;
                GameRenderer.ClearMenuPointer();
                return;
            }
        }

        if (
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
            (
                KeyDown &&
                Event.key.scancode == SDL_SCANCODE_ESCAPE
            )
        )
        {
            State.Paused = false;
            GameRenderer.ClearMenuPointer();
        }

        return;
    }

    if (
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

    if (
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

    if (KeyDown)
    {
        if (Event.key.scancode == SDL_SCANCODE_E)
            InteractPressed = true;

        if (Event.key.scancode == SDL_SCANCODE_R)
            RestartPressed = true;
    }
}

void Game::OnMouseCaptureChanged(bool Captured)
{
    GamePlayer.OnMouseCaptureChanged(Captured);
}

bool Game::ShouldCaptureMouse() const
{
    return
        State.Started &&
        !State.MainMenuOpen &&
        !State.Paused &&
        !MapOpen &&
        !State.Ended;
}

AABB Game::BreakerBounds(const Breaker& BreakerData) const
{
    const glm::vec3 Forward = BreakerData.Forward;
    const glm::vec3 Right{
        Forward.z,
        0.0f,
        -Forward.x
    };

    const float HalfX =
        std::abs(Right.x) * 0.35f +
        std::abs(Forward.x) * 0.36f;

    const float HalfZ =
        std::abs(Right.z) * 0.35f +
        std::abs(Forward.z) * 0.36f;

    return {
        {
            BreakerData.Position.x - HalfX,
            BreakerData.Position.y,
            BreakerData.Position.z - HalfZ
        },
        {
            BreakerData.Position.x + HalfX,
            BreakerData.Position.y + 0.94f,
            BreakerData.Position.z + HalfZ
        }
    };
}

AABB Game::BreakerInteractionBounds(const Breaker& BreakerData) const
{
    const AABB Physical = BreakerBounds(BreakerData);
    const glm::vec3 Expansion{0.12f, 0.12f, 0.12f};

    return {
        Physical.Min - Expansion,
        Physical.Max + Expansion
    };
}

AABB Game::ExitBounds() const
{
    const bool FacingX =
        std::abs(ExitForward.x) > 0.5f;

    const glm::vec3 Center =
        ExitPosition + glm::vec3{0.0f, 1.27f, 0.0f};

    const glm::vec3 HalfExtents =
        FacingX
            ? glm::vec3{0.20f, 1.27f, 0.74f}
            : glm::vec3{0.74f, 1.27f, 0.20f};

    return {
        Center - HalfExtents,
        Center + HalfExtents
    };
}

void Game::UpdateInteraction()
{
    InteractionType = 0;
    InteractionIndex = -1;

    const Ray ViewRay{
        GamePlayer.Position(),
        GamePlayer.Forward()
    };

    const float MaxDistance = 2.85f;

    const RayHit WallHit =
        Raycast::AgainstWorld(
            ViewRay,
            World.Colliders,
            MaxDistance
        );

    float BestDistance =
        WallHit.Hit
            ? WallHit.Distance
            : MaxDistance;

    for (std::size_t I = 0; I < Breakers.size(); ++I)
    {
        if (Breakers[I].Active)
            continue;

        const RayHit Hit = Raycast::AgainstAABB(
            ViewRay,
            BreakerInteractionBounds(Breakers[I]),
            BestDistance
        );

        if (!Hit.Hit)
            continue;

        BestDistance = Hit.Distance;
        InteractionType = 1;
        InteractionIndex = static_cast<int>(I);
    }

    const RayHit ExitHit =
        Raycast::AgainstAABB(
            ViewRay,
            ExitBounds(),
            BestDistance
        );

    if (ExitHit.Hit)
    {
        InteractionType = 2;
        InteractionIndex = -1;
    }
}

void Game::Interact()
{
    if (State.Ended)
        return;

    if (
        InteractionType == 1 &&
        InteractionIndex >= 0 &&
        InteractionIndex < static_cast<int>(Breakers.size())
    )
    {
        Breaker& Target =
            Breakers[static_cast<std::size_t>(InteractionIndex)];

        if (Target.Active)
            return;

        Target.Active = true;
        Audio.PlayBreaker(Target.Position);
        State.ActivateBreaker();

        if (State.BreakersActive == 1)
        {
            Hunter.Release();
            Audio.PlayEntityRelease(Hunter.Position());
            Message = "SOMETHING HEARD THAT";
        }
        else if (State.CanExit())
        {
            Message = "EXIT POWER RESTORED";
        }
        else
        {
            Message = "BREAKER ONLINE";
        }

        MessageTimer = 1.7f;

        UpdateTitle();

        return;
    }

    if (
        InteractionType == 2 &&
        State.CanExit()
    )
    {
        EndGame(true);
    }
}

void Game::EndGame(bool Escaped)
{
    State.Ended = true;
    State.Escaped = Escaped;

    if (!Escaped)
        Audio.PlayDeath();

    UpdateTitle();
}

void Game::Update(
    float DeltaTime,
    bool MouseCaptured
)
{
    GameRenderer.UpdateInterface(DeltaTime);


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
                88.0f / std::max(MapZoom, 0.45f);
            MapCenter += Pan * PanSpeed * DeltaTime;
        }

        InteractPressed = false;
        RestartPressed = false;
        return;
    }

    if (
        !State.Started ||
        State.MainMenuOpen ||
        State.Paused
    )
    {
        InteractPressed = false;
        RestartPressed = false;
        return;
    }

    if (State.Ended)
    {
        if (RestartPressed)
            Reset();

        RestartPressed = false;
        InteractPressed = false;
        return;
    }

    GamePlayer.Update(
        DeltaTime,
        World.Colliders,
        MouseCaptured
    );

    UpdateWaypoint(DeltaTime);

    WorldGenerator Streamer(Seed);

    if (Streamer.NeedsRebuild(World, GamePlayer.Position()))
    {
        WorldData StreamedWorld =
            Streamer.BuildAround(GamePlayer.Position());

        for (const Breaker& BreakerData : Breakers)
        {
            StreamedWorld.Colliders.push_back(
                BreakerBounds(BreakerData)
            );
        }

        StreamedWorld.Colliders.push_back(ExitBounds());
        World = std::move(StreamedWorld);
    }

    glm::vec3 PlayerTravel =
        GamePlayer.Position() - PreviousPlayerPosition;
    PlayerTravel.y = 0.0f;

    const float TravelDistance = glm::length(PlayerTravel);

    if (TravelDistance > 0.0001f)
    {
        FootstepDistance += TravelDistance;

        while (FootstepDistance >= 1.28f)
        {
            FootstepDistance -= 1.28f;
            Audio.PlayFootstep(GamePlayer.Position());
        }
    }

    PreviousPlayerPosition = GamePlayer.Position();

    UpdateInteraction();

    if (InteractPressed)
        Interact();

    InteractPressed = false;
    RestartPressed = false;

    if (State.EntityReleased)
    {
        const bool Caught = Hunter.Update(
            DeltaTime,
            GamePlayer.Position(),
            World
        );

        if (Hunter.ConsumeShifted())
        {
            Audio.PlayShift(
                Hunter.IsDemonForm(),
                Hunter.Position()
            );
        }

        if (Caught)
            EndGame(false);
    }

    if (MessageTimer > 0.0f)
    {
        MessageTimer = std::max(
            0.0f,
            MessageTimer - DeltaTime
        );

        if (MessageTimer <= 0.0f)
            Message.clear();
    }

    Audio.Update(
        DeltaTime,
        GamePlayer.Position(),
        GamePlayer.Forward(),
        Hunter.Position(),
        Hunter.IsActive()
    );
    UpdateTitle();
}

std::vector<SceneBox> Game::BuildDynamicBoxes() const
{
    std::vector<SceneBox> Boxes;

    if (!GameRenderer.HasBreakerModel())
    {
        for (const Breaker& BreakerData : Breakers)
        {
            const glm::vec3 Forward = BreakerData.Forward;
            const glm::vec3 Right{
                Forward.z,
                0.0f,
                -Forward.x
            };

            const bool FacingX =
                std::abs(Forward.x) > 0.5f;

            auto OrientedSize = [&](
                float Width,
                float Height,
                float Depth
            )
            {
                return FacingX
                    ? glm::vec3{Depth, Height, Width}
                    : glm::vec3{Width, Height, Depth};
            };

            auto AddPiece = [&](
                const glm::vec3& Position,
                float Width,
                float Height,
                float Depth,
                const glm::vec3& Color,
                const glm::vec3& Emissive,
                float Roughness
            )
            {
                Boxes.push_back({
                    Position,
                    OrientedSize(Width, Height, Depth),
                    Color,
                    Emissive,
                    Roughness,
                    static_cast<int>(SurfaceMaterial::Fixture)
                });
            };

            AddPiece(
                BreakerData.Position +
                    glm::vec3{0.0f, 0.48f, 0.0f},
                0.66f,
                0.96f,
                0.18f,
                {0.19f, 0.20f, 0.18f},
                {0.0f, 0.0f, 0.0f},
                0.62f
            );

            AddPiece(
                BreakerData.Position +
                    Forward * 0.12f +
                    glm::vec3{0.0f, 0.48f, 0.0f},
                0.54f,
                0.80f,
                0.055f,
                {0.34f, 0.35f, 0.31f},
                {0.0f, 0.0f, 0.0f},
                0.48f
            );

            for (int Row = 0; Row < 3; ++Row)
            {
                for (int Column = 0; Column < 2; ++Column)
                {
                    const float Side =
                        Column == 0 ? -0.14f : 0.14f;

                    AddPiece(
                        BreakerData.Position +
                            Right * Side +
                            Forward * 0.17f +
                            glm::vec3{
                                0.0f,
                                0.28f + static_cast<float>(Row) * 0.20f,
                                0.0f
                            },
                        0.105f,
                        0.11f,
                        0.075f,
                        {0.055f, 0.057f, 0.052f},
                        {0.0f, 0.0f, 0.0f},
                        0.34f
                    );
                }
            }

            AddPiece(
                BreakerData.Position +
                    Forward * 0.18f +
                    glm::vec3{0.0f, 0.84f, 0.0f},
                0.075f,
                0.075f,
                0.06f,
                BreakerData.Active
                    ? glm::vec3{0.10f, 0.38f, 0.12f}
                    : glm::vec3{0.42f, 0.08f, 0.05f},
                BreakerData.Active
                    ? glm::vec3{0.02f, 0.20f, 0.03f}
                    : glm::vec3{0.18f, 0.015f, 0.008f},
                0.25f
            );
        }
    }

    if (GameRenderer.HasBreakerModel())
{
    for (const Breaker& BreakerData : Breakers)
    {
        const glm::vec3 Forward = BreakerData.Forward;
        const bool FacingX = std::abs(Forward.x) > 0.5f;

        const glm::vec3 BackingSize = FacingX
            ? glm::vec3{0.035f, 0.13f, 0.17f}
            : glm::vec3{0.17f, 0.13f, 0.035f};

        const glm::vec3 LightSize = FacingX
            ? glm::vec3{0.028f, 0.078f, 0.078f}
            : glm::vec3{0.078f, 0.078f, 0.028f};

        Boxes.push_back({
            BreakerData.Position -
                Forward * 0.155f +
                glm::vec3{0.0f, 0.76f, 0.0f},
            BackingSize,
            {0.035f, 0.038f, 0.033f},
            {0.0f, 0.0f, 0.0f},
            0.42f,
            static_cast<int>(SurfaceMaterial::Fixture)
        });

        Boxes.push_back({
            BreakerData.Position -
                Forward * 0.13f +
                glm::vec3{0.0f, 0.76f, 0.0f},
            LightSize,
            BreakerData.Active
                ? glm::vec3{0.10f, 0.46f, 0.13f}
                : glm::vec3{0.50f, 0.07f, 0.045f},
            BreakerData.Active
                ? glm::vec3{0.035f, 0.30f, 0.055f}
                : glm::vec3{0.28f, 0.012f, 0.006f},
            0.18f,
            static_cast<int>(SurfaceMaterial::Fixture)
        });
    }
}

    if (!GameRenderer.HasExitDoorModel())
    {
        const glm::vec3 ExitRight{
        ExitForward.z,
        0.0f,
        -ExitForward.x
    };

    const bool ExitFacingX =
        std::abs(ExitForward.x) > 0.5f;

    auto ExitSize = [&](
        float Width,
        float Height,
        float Depth
    )
    {
        return ExitFacingX
            ? glm::vec3{Depth, Height, Width}
            : glm::vec3{Width, Height, Depth};
    };

    auto AddExitPiece = [&](
        const glm::vec3& Position,
        float Width,
        float Height,
        float Depth,
        const glm::vec3& Color,
        const glm::vec3& Emissive,
        float Roughness
    )
    {
        Boxes.push_back({
            Position,
            ExitSize(Width, Height, Depth),
            Color,
            Emissive,
            Roughness,
            static_cast<int>(SurfaceMaterial::Fixture)
        });
    };

    AddExitPiece(
        ExitPosition + glm::vec3{0.0f, 1.20f, 0.0f},
        1.22f,
        2.40f,
        0.14f,
        {0.245f, 0.235f, 0.185f},
        {0.0f, 0.0f, 0.0f},
        0.72f
    );

    AddExitPiece(
        ExitPosition +
            ExitForward * 0.082f +
            glm::vec3{0.0f, 1.22f, 0.0f},
        0.92f,
        1.78f,
        0.035f,
        {0.17f, 0.165f, 0.13f},
        {0.0f, 0.0f, 0.0f},
        0.66f
    );

    AddExitPiece(
        ExitPosition -
            ExitRight * 0.69f +
            glm::vec3{0.0f, 1.25f, 0.0f},
        0.13f,
        2.50f,
        0.23f,
        {0.72f, 0.69f, 0.54f},
        {0.0f, 0.0f, 0.0f},
        0.58f
    );

    AddExitPiece(
        ExitPosition +
            ExitRight * 0.69f +
            glm::vec3{0.0f, 1.25f, 0.0f},
        0.13f,
        2.50f,
        0.23f,
        {0.72f, 0.69f, 0.54f},
        {0.0f, 0.0f, 0.0f},
        0.58f
    );

    AddExitPiece(
        ExitPosition + glm::vec3{0.0f, 2.46f, 0.0f},
        1.51f,
        0.13f,
        0.23f,
        {0.72f, 0.69f, 0.54f},
        {0.0f, 0.0f, 0.0f},
        0.58f
    );

    AddExitPiece(
        ExitPosition +
            ExitRight * 0.42f +
            ExitForward * 0.13f +
            glm::vec3{0.0f, 1.05f, 0.0f},
        0.055f,
        0.20f,
        0.075f,
        {0.58f, 0.56f, 0.43f},
        {0.0f, 0.0f, 0.0f},
        0.22f
    );

    AddExitPiece(
        ExitPosition +
            ExitForward * 0.14f +
            glm::vec3{0.0f, 2.20f, 0.0f},
        0.18f,
        0.075f,
        0.055f,
        State.CanExit()
            ? glm::vec3{0.10f, 0.44f, 0.12f}
            : glm::vec3{0.48f, 0.09f, 0.055f},
        State.CanExit()
            ? glm::vec3{0.025f, 0.24f, 0.035f}
            : glm::vec3{0.22f, 0.018f, 0.009f},
        0.25f
    );

    }

    if (!GameRenderer.HasEntityModels())
    {
        const std::vector<SceneBox> EntityBoxes =
            Hunter.BuildRenderBoxes();

        Boxes.insert(
            Boxes.end(),
            EntityBoxes.begin(),
            EntityBoxes.end()
        );
    }

    return Boxes;
}

void Game::Render(float Time)
{
    const uint64_t CurrentFpsCounter =
        SDL_GetPerformanceCounter();

    if (FpsCounterStart == 0)
    {
        FpsCounterStart = CurrentFpsCounter;
        FrameCounter = 0;
    }

    ++FrameCounter;

    const double FpsFrequency =
        static_cast<double>(SDL_GetPerformanceFrequency());

    const double FpsElapsed =
        static_cast<double>(
            CurrentFpsCounter - FpsCounterStart
        ) /
        std::max(FpsFrequency, 1.0);

    if (FpsElapsed >= 0.15)
    {
        DisplayedFps =
            static_cast<float>(
                static_cast<double>(FrameCounter) /
                std::max(FpsElapsed, 0.001)
            );

        FrameCounter = 0;
        FpsCounterStart = CurrentFpsCounter;
    }

    GameRenderer.BeginFrame();

    const float Aspect =
        static_cast<float>(Width) /
        static_cast<float>(std::max(Height, 1u));

    GameRenderer.SetCamera(
        GamePlayer.ViewMatrix(),
        GamePlayer.ProjectionMatrix(Aspect),
        GamePlayer.Position()
    );

    GameRenderer.SetLights(
        World.Lights,
        GamePlayer.Position(),
        Time
    );

    GameRenderer.DrawBoxes(World.Boxes);

    const std::vector<SceneBox> Dynamic =
        BuildDynamicBoxes();

    GameRenderer.DrawBoxes(Dynamic);

    if (GameRenderer.HasBreakerModel())
    {
        for (const Breaker& BreakerData : Breakers)
        {
            GameRenderer.DrawBreaker(
                BreakerData.Position,
                BreakerData.Forward
            );
        }
    }

    if (GameRenderer.HasExitDoorModel())
    {
        GameRenderer.DrawExitDoor(
            ExitPosition,
            ExitForward
        );
    }

    if (
        Hunter.IsActive() &&
        GameRenderer.HasEntityModels()
    )
    {
        GameRenderer.DrawEntity(
            Hunter.Position(),
            Hunter.Forward(),
            Hunter.IsDemonForm(),
            Hunter.PreviousWasDemonForm(),
            Hunter.ShiftAmount()
        );
    }

    GameRenderer.EndFrame(
        GamePlayer.Stamina(),
        State.BreakersActive,
        State.BreakersRequired,
        InteractionType,
        State.CanExit(),
        DisplayedFps,
        Message,
        State.Started,
        State.MainMenuOpen,
        State.Paused,
        State.Ended,
        State.Escaped
    );

    if (
        State.Started &&
        !State.MainMenuOpen &&
        !State.Paused &&
        !State.Ended
    )
    {
        GameRenderer.DrawGameplayOverlayV3(
            GamePlayer.Stamina(),
            State.BreakersActive,
            State.BreakersRequired,
            InteractionType,
            State.CanExit(),
            DisplayedFps
        );

        MapWaypointView Waypoint;
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
            GameRenderer.DrawTouchControlsV1();
    }
}

void Game::RenderUpdateScreen(
    const UpdateVisualState& State
)
{
    GameRenderer.BeginFrame();
    GameRenderer.DrawUpdateScreen(State);
}

void Game::UpdateTitle()
{
    Title = "Backrooms Offical";
}

const std::string& Game::WindowTitle() const
{
    return Title;
}
