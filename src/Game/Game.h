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
