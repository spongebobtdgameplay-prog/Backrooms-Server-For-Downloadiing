#pragma once

#include <glm/glm.hpp>

#include <array>

struct ma_engine;
struct ma_sound;

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    bool Initialize();
    void Shutdown();

    void Update(
        float DeltaTime,
        const glm::vec3& ListenerPosition,
        const glm::vec3& ListenerForward,
        const glm::vec3& EntityPosition,
        bool EntityActive
    );

    void PlayBreaker(const glm::vec3& Position);
    void PlayFootstep(const glm::vec3& Position);
    void PlayShift(bool DemonForm, const glm::vec3& Position);
    void PlayDeath();

private:
    ma_sound* LoadSound(
        const char* Path,
        bool Looping,
        bool Spatial,
        float Volume
    );

    void RestartSpatial(
        ma_sound* Sound,
        const glm::vec3& Position,
        float Volume,
        float Pitch = 1.0f
    );

    ma_engine* Engine = nullptr;
    ma_sound* Hum = nullptr;
    ma_sound* BreakerTrip = nullptr;
    ma_sound* EntityMetal = nullptr;
    ma_sound* Shift = nullptr;
    ma_sound* EntityLaugh = nullptr;
    std::array<ma_sound*, 4> Footsteps{};

    float EntityCueTimer = 3.5f;
    int EntityCueIndex = 0;
    int FootstepIndex = 0;

    bool Started = false;
};
