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
    GameRenderer.Shutdown();
}

void Game::Resize(uint32_t NewWidth, uint32_t NewHeight)
{
    Width = std::max(NewWidth, 1u);
    Height = std::max(NewHeight, 1u);

    GameRenderer.Resize(Width, Height);
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
        Breakers.push_back({
            Pick(24.0f),
            false
        });
    }

    ExitPosition = Pick(38.0f);

    const glm::vec3 EntityPosition = Pick(50.0f);
    Hunter.Reset(EntityPosition);

    InteractionType = 0;
    InteractionIndex = -1;

    InteractPressed = false;
    RestartPressed = false;

    FrameCounter = 0;
    FpsWindowStart = 0.0f;
    DisplayedFps = 0.0f;

    Message.clear();
    MessageTimer = 0.0f;

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
        Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        (
            KeyDown &&
            (
                Event.key.scancode == SDL_SCANCODE_RETURN ||
                Event.key.scancode == SDL_SCANCODE_SPACE
            )
        );

    if (State.MainMenuOpen)
    {
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
            return;
        }

        if (Activate)
        {
            State.Started = true;
            State.MainMenuOpen = false;
            State.Paused = false;
        }

        return;
    }

    if (State.Paused)
    {
        if (
            KeyDown &&
            Event.key.scancode == SDL_SCANCODE_M
        )
        {
            State.Paused = false;
            State.MainMenuOpen = true;
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
    return {
        BreakerData.Position + glm::vec3{-0.42f, 0.72f, -0.3f},
        BreakerData.Position + glm::vec3{0.42f, 1.88f, 0.3f}
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

    UpdateInteraction();

    if (InteractPressed)
        Interact();

    InteractPressed = false;
    RestartPressed = false;

    float EntityDistance =
        std::numeric_limits<float>::infinity();

    if (State.EntityReleased)
    {
        const bool Caught = Hunter.Update(
            DeltaTime,
            GamePlayer.Position(),
            World
        );

        EntityDistance =
            Hunter.DistanceTo(GamePlayer.Position());

        if (Hunter.ConsumeShifted())
        {
            Audio.PlayShift(
                Hunter.IsDemonForm()
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

    Audio.Update(EntityDistance);
    UpdateTitle();
}

std::vector<SceneBox> Game::BuildDynamicBoxes() const
{
    std::vector<SceneBox> Boxes;

    for (const Breaker& BreakerData : Breakers)
    {
        Boxes.push_back({
            BreakerData.Position + glm::vec3{0.0f, 1.35f, 0.0f},
            {0.7f, 1.0f, 0.18f},
            {0.0409f, 0.0382f, 0.0203f},
            {0.0f, 0.0f, 0.0f},
            0.8f
        });

        Boxes.push_back({
            BreakerData.Position + glm::vec3{0.0f, 1.35f, -0.14f},
            {0.18f, 0.34f, 0.1f},
            BreakerData.Active
                ? glm::vec3{0.0437f, 0.4564f, 0.0723f}
                : glm::vec3{0.4735f, 0.0513f, 0.0273f},
            BreakerData.Active
                ? glm::vec3{0.00212f, 0.04667f, 0.00518f}
                : glm::vec3{0.04231f, 0.00273f, 0.00121f},
            0.7f
        });
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
    ++FrameCounter;

    if (FpsWindowStart <= 0.0f)
        FpsWindowStart = Time;

    const float FpsElapsed = Time - FpsWindowStart;

    if (FpsElapsed >= 0.5f)
    {
        DisplayedFps =
            static_cast<float>(FrameCounter) /
            std::max(FpsElapsed, 0.001f);

        FrameCounter = 0;
        FpsWindowStart = Time;
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

    if (
        Hunter.IsActive() &&
        GameRenderer.HasEntityModels()
    )
    {
        GameRenderer.DrawEntity(
            Hunter.Position(),
            Hunter.Forward(),
            Hunter.IsDemonForm()
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
