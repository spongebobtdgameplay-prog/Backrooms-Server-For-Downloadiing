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

    void OnMouseCaptureChanged(bool Captured);
    bool ShouldCaptureMouse() const;

    void Update(
        float DeltaTime,
        bool MouseCaptured
    );

    void Render(float Time);
    void RenderUpdateScreen(const UpdateVisualState& State);

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
