#include "Game.h"

#include "../Physics/Raycast.h"
#include "../Updater/UpdaterService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <sstream>
#include <unordered_set>

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
    const int X = std::clamp(
        static_cast<int>(std::round(CellCenter.x / World.CellSize)),
        0,
        World.Columns - 1
    );

    const int Z = std::clamp(
        static_cast<int>(std::round(CellCenter.z / World.CellSize)),
        0,
        World.Rows - 1
    );

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
            static_cast<uint32_t>((X + 1) * 3266489917u) ^
            static_cast<uint32_t>((Z + 1) * 668265263u);

        const float Along =
            (static_cast<float>(Hash % 1000u) / 999.0f - 0.5f) *
            1.45f;

        Result.Position =
            WallCenter +
            Forward * (World.WallThickness * 0.5f + 0.11f) +
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
    World = Generator.Build();

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

    ExitPosition = Pick(38.0f);

    const glm::vec3 EntityPosition = Pick(50.0f);
    Hunter.Reset(EntityPosition);

    InteractionType = 0;
    InteractionIndex = -1;

    InteractPressed = false;
    RestartPressed = false;

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

        if (Activate)
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
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_M
        )
        {
            State.Paused = false;
            State.MainMenuOpen = true;
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
        KeyDown &&
        Event.key.scancode == SDL_SCANCODE_ESCAPE
    )
    {
        State.Paused = true;
        GameRenderer.ClearMenuPointer();
        return;
    }

    if (
        State.Started &&
        !State.Ended &&
        KeyDown &&
        Event.key.scancode == SDL_SCANCODE_M
    )
    {
        State.MainMenuOpen = true;
        State.Paused = false;
        GameRenderer.ClearMenuPointer();
        return;
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
        std::abs(Right.x) * 0.40f +
        std::abs(Forward.x) * 0.18f;

    const float HalfZ =
        std::abs(Right.z) * 0.40f +
        std::abs(Forward.z) * 0.18f;

    return {
        {
            BreakerData.Position.x - HalfX,
            BreakerData.Position.y,
            BreakerData.Position.z - HalfZ
        },
        {
            BreakerData.Position.x + HalfX,
            BreakerData.Position.y + 1.05f,
            BreakerData.Position.z + HalfZ
        }
    };
}

AABB Game::ExitBounds() const
{
    return {
        ExitPosition + glm::vec3{-0.95f, 0.0f, -0.32f},
        ExitPosition + glm::vec3{0.95f, 2.7f, 0.32f}
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

    const float MaxDistance = 2.5f;

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
            BreakerBounds(Breakers[I]),
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
            const bool FacingX =
                std::abs(BreakerData.Forward.x) > 0.5f;

            Boxes.push_back({
                BreakerData.Position + glm::vec3{0.0f, 0.525f, 0.0f},
                FacingX
                    ? glm::vec3{0.18f, 1.05f, 0.76f}
                    : glm::vec3{0.76f, 1.05f, 0.18f},
                {0.16f, 0.17f, 0.15f},
                {0.0f, 0.0f, 0.0f},
                0.88f,
                static_cast<int>(SurfaceMaterial::Fixture)
            });
        }
    }

    Boxes.push_back({
        ExitPosition + glm::vec3{0.0f, 1.3f, 0.0f},
        {1.8f, 2.6f, 0.3f},
        {0.0185f, 0.0185f, 0.0116f},
        {0.0f, 0.0f, 0.0f},
        0.75f
    });

    Boxes.push_back({
        ExitPosition + glm::vec3{0.0f, 1.18f, -0.2f},
        {1.38f, 2.25f, 0.08f},
        {0.2423f, 0.2159f, 0.0762f},
        State.CanExit()
            ? glm::vec3{0.00802f, 0.04667f, 0.00857f}
            : glm::vec3{0.0f},
        0.9f
    });

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
