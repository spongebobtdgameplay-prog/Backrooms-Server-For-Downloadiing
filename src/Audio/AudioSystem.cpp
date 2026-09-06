#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

AudioSystem::AudioSystem() = default;
AudioSystem::~AudioSystem() { Shutdown(); }

ma_sound* AudioSystem::LoadSound(const char* Path, bool Looping, bool Spatial, float Volume)
{
    if (Engine == nullptr || Path == nullptr || !std::filesystem::exists(Path))
        return nullptr;

    ma_sound* Sound = new ma_sound;
    if (ma_sound_init_from_file(Engine, Path, 0, nullptr, nullptr, Sound) != MA_SUCCESS)
    {
        delete Sound;
        return nullptr;
    }

    ma_sound_set_looping(Sound, Looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_spatialization_enabled(Sound, Spatial ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(Sound, Volume);
    if (Spatial)
    {
        ma_sound_set_attenuation_model(Sound, ma_attenuation_model_inverse);
        ma_sound_set_min_distance(Sound, 1.2f);
        ma_sound_set_max_distance(Sound, 34.0f);
        ma_sound_set_rolloff(Sound, 0.85f);
    }
    return Sound;
}

bool AudioSystem::Initialize()
{
    if (Started)
        return true;

    Engine = new ma_engine;
    if (ma_engine_init(nullptr, Engine) != MA_SUCCESS)
    {
        delete Engine;
        Engine = nullptr;
        return false;
    }

    Hum = LoadSound("assets/audio/fluorescent-hum.wav", true, false, 0.24f);
    if (Hum == nullptr)
        Hum = LoadSound("assets/audio/hum.wav", true, false, 0.18f);
    BreakerTrip = LoadSound("assets/audio/breaker-trip.wav", false, true, 0.65f);

    for (std::size_t I = 0; I < Footsteps.size(); ++I)
    {
        const std::string Path = "assets/audio/footstep-carpet-" + std::to_string(I + 1) + ".wav";
        Footsteps[I] = LoadSound(Path.c_str(), false, true, 0.32f);
        EntityFootsteps[I] = LoadSound(Path.c_str(), false, true, 0.64f);

        if (Footsteps[I] != nullptr)
        {
            ma_sound_set_min_distance(Footsteps[I], 0.5f);
            ma_sound_set_max_distance(Footsteps[I], 12.0f);
            ma_sound_set_rolloff(Footsteps[I], 0.65f);
        }
        if (EntityFootsteps[I] != nullptr)
        {
            ma_sound_set_min_distance(EntityFootsteps[I], 1.4f);
            ma_sound_set_max_distance(EntityFootsteps[I], 50.0f);
            ma_sound_set_rolloff(EntityFootsteps[I], 0.48f);
        }
    }

    if (Hum != nullptr)
        ma_sound_start(Hum);

    PreviousEntityPosition = {0.0f, 0.0f, 0.0f};
    EntityFootstepDistance = 0.0f;
    EntityFootstepIndex = 0;
    FootstepIndex = 0;
    TrackingEntity = false;
    Started = true;
    return true;
}

void AudioSystem::Shutdown()
{
    if (!Started)
        return;

    auto Release = [](ma_sound*& Sound)
    {
        if (Sound == nullptr)
            return;
        ma_sound_uninit(Sound);
        delete Sound;
        Sound = nullptr;
    };

    Release(Hum);
    Release(BreakerTrip);
    for (ma_sound*& Sound : Footsteps) Release(Sound);
    for (ma_sound*& Sound : EntityFootsteps) Release(Sound);

    if (Engine != nullptr)
    {
        ma_engine_uninit(Engine);
        delete Engine;
        Engine = nullptr;
    }
    Started = false;
}

void AudioSystem::RestartSpatial(ma_sound* Sound, const glm::vec3& Position, float Volume, float Pitch)
{
    if (!Started || Sound == nullptr)
        return;
    ma_sound_stop(Sound);
    ma_sound_seek_to_pcm_frame(Sound, 0);
    ma_sound_set_position(Sound, Position.x, Position.y, Position.z);
    ma_sound_set_volume(Sound, Volume);
    ma_sound_set_pitch(Sound, Pitch);
    ma_sound_start(Sound);
}

void AudioSystem::PlayEntityStep(const glm::vec3& Position, float Threat)
{
    ma_sound* Sound = EntityFootsteps[static_cast<std::size_t>(EntityFootstepIndex % static_cast<int>(EntityFootsteps.size()))];
    const float Pitch = 0.72f + static_cast<float>(EntityFootstepIndex % 3) * 0.035f;
    const float Volume = 0.52f + std::clamp(Threat, 0.0f, 1.0f) * 0.30f;
    ++EntityFootstepIndex;
    RestartSpatial(Sound, Position + glm::vec3{0.0f, 0.08f, 0.0f}, Volume, Pitch);
}

void AudioSystem::Update(
    float DeltaTime,
    const glm::vec3& ListenerPosition,
    const glm::vec3& ListenerForward,
    const glm::vec3& EntityPosition,
    bool EntityActive
)
{
    if (!Started || Engine == nullptr)
        return;

    ma_engine_listener_set_position(Engine, 0, ListenerPosition.x, ListenerPosition.y, ListenerPosition.z);
    ma_engine_listener_set_direction(Engine, 0, ListenerForward.x, ListenerForward.y, ListenerForward.z);
    ma_engine_listener_set_world_up(Engine, 0, 0.0f, 1.0f, 0.0f);

    float Distance = 100.0f;
    if (EntityActive)
        Distance = glm::length(EntityPosition - ListenerPosition);
    const float Threat = EntityActive
        ? std::clamp(1.0f - (Distance - 3.0f) / 34.0f, 0.0f, 1.0f)
        : 0.0f;

    if (Hum != nullptr)
        ma_sound_set_volume(Hum, 0.22f + Threat * 0.025f);

    if (!EntityActive)
    {
        TrackingEntity = false;
        EntityFootstepDistance = 0.0f;
        return;
    }

    if (!TrackingEntity)
    {
        PreviousEntityPosition = EntityPosition;
        TrackingEntity = true;
        return;
    }

    glm::vec3 Travel = EntityPosition - PreviousEntityPosition;
    Travel.y = 0.0f;
    const float TravelDistance = glm::length(Travel);
    PreviousEntityPosition = EntityPosition;

    if (TravelDistance > 0.0001f && TravelDistance < 4.0f)
    {
        EntityFootstepDistance += TravelDistance;
        const float StepSpacing = 0.86f + (1.0f - Threat) * 0.16f;
        while (EntityFootstepDistance >= StepSpacing)
        {
            EntityFootstepDistance -= StepSpacing;
            PlayEntityStep(EntityPosition, Threat);
        }
    }

    static_cast<void>(DeltaTime);
}

void AudioSystem::PlayEntityRelease(const glm::vec3& Position)
{
    PreviousEntityPosition = Position;
    EntityFootstepDistance = 0.0f;
    TrackingEntity = true;
    PlayEntityStep(Position, 0.25f);
}

void AudioSystem::PlayBreaker(const glm::vec3& Position)
{
    RestartSpatial(BreakerTrip, Position + glm::vec3{0.0f, 0.55f, 0.0f}, 0.70f, 0.93f);
}

void AudioSystem::PlayFootstep(const glm::vec3& Position)
{
    if (!Started)
        return;
    ma_sound* Sound = Footsteps[static_cast<std::size_t>(FootstepIndex % static_cast<int>(Footsteps.size()))];
    const float Pitch = 0.95f + static_cast<float>(FootstepIndex % 3) * 0.035f;
    ++FootstepIndex;
    RestartSpatial(Sound, Position - glm::vec3{0.0f, 1.45f, 0.0f}, 0.32f, Pitch);
}

void AudioSystem::PlayShift(bool DemonForm, const glm::vec3& Position)
{
    static_cast<void>(DemonForm);
    static_cast<void>(Position);
}

void AudioSystem::PlayDeath()
{
    if (!Started || Engine == nullptr)
        return;
    if (std::filesystem::exists("assets/audio/entity-death.wav"))
    {
        ma_engine_play_sound(Engine, "assets/audio/entity-death.wav", nullptr);
        return;
    }
    if (std::filesystem::exists("assets/audio/death.wav"))
        ma_engine_play_sound(Engine, "assets/audio/death.wav", nullptr);
}
