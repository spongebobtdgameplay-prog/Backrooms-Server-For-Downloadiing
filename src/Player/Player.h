#pragma once

#include "../World/WorldTypes.h"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

class Player
{
public:
    void Reset(const glm::vec3& Position);

    void HandleEvent(const SDL_Event& Event, bool MouseCaptured);
    void Update(
        float DeltaTime,
        const std::vector<AABB>& Colliders,
        bool MouseCaptured
    );

    glm::vec3 Forward() const;
    glm::vec3 Right() const;
    glm::mat4 ViewMatrix() const;
    glm::mat4 ProjectionMatrix(float Aspect) const;

    const glm::vec3& Position() const;
    float Stamina() const;
    float Fov() const;

private:
    glm::vec3 PlayerPosition{0.0f, 1.65f, 0.0f};

    float Yaw = 0.0f;
    float Pitch = 0.0f;
    float TargetYaw = 0.0f;
    float TargetPitch = 0.0f;

    float MouseDeltaX = 0.0f;
    float MouseDeltaY = 0.0f;

    float Bob = 0.0f;
    float BobTime = 0.0f;
    float Roll = 0.0f;

    float Radius = 0.38f;
    float WalkSpeed = 4.3f;
    float SprintSpeed = 6.8f;
    float CurrentStamina = 1.0f;
    float CurrentFov = 73.0f;
};
