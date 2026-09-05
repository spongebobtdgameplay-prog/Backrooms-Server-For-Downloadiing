#pragma once

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

    bool HandleMenuPointer(float X, float Y)
    {
        if (State.MainMenuOpen)
        {
            const auto Layout = MenuLayout::BuildMainMenu(
                Width,
                Height,
                State.Started
            );

            if (Layout.PrimaryButton.Contains(X, Y))
            {
                State.Started = true;
                State.MainMenuOpen = false;
                State.Paused = false;
                return true;
            }

            if (
                Layout.HasSecondaryButton &&
                Layout.SecondaryButton.Contains(X, Y)
            )
            {
                Reset();
                State.Started = true;
                State.MainMenuOpen = false;
                State.Paused = false;
                return true;
            }

            return false;
        }

        if (State.Paused)
        {
            const auto Layout = MenuLayout::BuildPauseMenu(
                Width,
                Height
            );

            if (Layout.ResumeButton.Contains(X, Y))
            {
                State.Paused = false;
                return true;
            }

            if (Layout.MainMenuButton.Contains(X, Y))
            {
                State.Paused = false;
                State.MainMenuOpen = true;
                return true;
            }
        }

        return false;
    }

    void OnMouseCaptureChanged(bool Captured);
    bool ShouldCaptureMouse() const;

    bool HasMenuOverlay() const
    {
        return State.MainMenuOpen || State.Paused;
    }

    void RenderMenuOverlay()
    {
        GameRenderer.BeginFrame();

        if (State.Paused)
            GameRenderer.DrawClassicPauseMenu();
        else
            GameRenderer.DrawClassicMainMenu(State.Started);
    }

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
        bool Active = false;
    };

    void Reset();
    glm::vec3 PickOpenCell(float MinDistance);

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

    int InteractionType = 0;
    int InteractionIndex = -1;

    bool InteractPressed = false;
    bool RestartPressed = false;

    int FrameCounter = 0;
    float FpsWindowStart = 0.0f;
    float DisplayedFps = 0.0f;

    std::string Message;
    float MessageTimer = 0.0f;

    std::string Title = "Backrooms Offical";
};