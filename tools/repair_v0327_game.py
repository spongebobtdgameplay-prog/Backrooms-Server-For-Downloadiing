from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text, old, new, label):
    if old not in text:
        raise RuntimeError(f"Missing anchor: {label}")
    return text.replace(old, new, 1)


def baseline_file(path):
    return subprocess.check_output(
        [
            "git",
            "show",
            f"origin/checkpoint-v0.3.26-before-map-ui-input:{path}",
        ],
        cwd=ROOT,
        text=True,
    )


game_cpp = baseline_file("src/Game/Game.cpp")

game_cpp = replace_once(
    game_cpp,
    '#include "Game.h"\n\n#include "../Physics/Raycast.h"',
    '#include "Game.h"\n#include "MapNavigation.h"\n\n#include "../Physics/Raycast.h"',
    "MapNavigation include",
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
    "shutdown input",
)

open_close_pattern = re.compile(
    r"void Game::OpenMap\(bool ReturnToPause\)\n\{.*?\n\}\n\nvoid Game::CloseMap\(\)\n\{.*?\n\}\n",
    re.S,
)
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
'''
game_cpp, count = open_close_pattern.subn(open_close_replacement, game_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace map open/close")

render_menu_pattern = re.compile(
    r"void Game::RenderMenuOverlay\(\)\n\{.*?\n\}\n\nglm::vec3 Game::MenuPointerFromEvent",
    re.S,
)
render_menu_replacement = '''void Game::RenderMenuOverlay()
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

glm::vec3 Game::MenuPointerFromEvent'''
game_cpp, count = render_menu_pattern.subn(render_menu_replacement, game_cpp, count=1)
if count != 1:
    raise RuntimeError("Could not replace RenderMenuOverlay")

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
game_cpp = replace_once(game_cpp, reset_old, reset_new, "reset state")

handle_pattern = re.compile(
    r"void Game::HandleEvent\(\n    const SDL_Event& Event,\n    bool MouseCaptured\n\)\n\{.*?\n\}\n\nvoid Game::OnMouseCaptureChanged",
    re.S,
)
match = handle_pattern.search(game_cpp)
if not match:
    raise RuntimeError("HandleEvent function not found")
handle = match.group(0)
handle_body = handle[: handle.rfind("\n\nvoid Game::OnMouseCaptureChanged")]

map_block_pattern = re.compile(
    r"\n    if \(MapOpen\)\n    \{.*?\n    \}\n\n    if \(State.MainMenuOpen\)",
    re.S,
)
map_block = r'''
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

    if (State.MainMenuOpen)'''
handle_body, count = map_block_pattern.subn(map_block, handle_body, count=1)
if count != 1:
    raise RuntimeError("Could not replace HandleEvent map block")

handle_body = replace_once(
    handle_body,
    '''        if (Activate)
        {
            State.Started = true;
            State.MainMenuOpen = false;
            State.Paused = false;
            GameRenderer.ClearMenuPointer();
        }
''',
    '''        if (
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
''',
    "main gamepad activate",
)

handle_body = replace_once(
    handle_body,
    '''        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_M
        )
        {
            OpenMap(true);
            return;
        }

        if (
            Activate ||
''',
    '''        if (
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
''',
    "pause gamepad controls",
)

handle_body = replace_once(
    handle_body,
    '''    if (
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
''',
    '''    if (
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
''',
    "gameplay menu input",
)

handle_body = replace_once(
    handle_body,
    '''    if (
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
''',
    '''    if (
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
''',
    "gameplay map/use device controls",
)

new_handle = handle_body + "\n\nvoid Game::OnMouseCaptureChanged"
game_cpp = game_cpp[: match.start()] + new_handle + game_cpp[match.end():]

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
game_cpp = replace_once(game_cpp, update_map_old, update_map_new, "map keyboard pan")

game_cpp = replace_once(
    game_cpp,
    '''    GamePlayer.Update(
        DeltaTime,
        World.Colliders,
        MouseCaptured
    );

    WorldGenerator Streamer(Seed);
''',
    '''    GamePlayer.Update(
        DeltaTime,
        World.Colliders,
        MouseCaptured
    );

    UpdateWaypoint(DeltaTime);

    WorldGenerator Streamer(Seed);
''',
    "waypoint update",
)

mini_pattern = re.compile(
    r"        GameRenderer\.DrawMiniMapV1\(\n            World,.*?\n        \);",
    re.S,
)
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

(ROOT / "src/Game/Game.cpp").write_text(game_cpp, encoding="utf-8")
print("Game.cpp repaired from checkpoint and V0.3.27 changes reapplied")
