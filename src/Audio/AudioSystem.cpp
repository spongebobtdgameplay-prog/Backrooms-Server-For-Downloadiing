#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem()
{
    Shutdown();
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

    Hum = new ma_sound;
    Static = new ma_sound;

    bool HumReady = false;
    bool StaticReady = false;

    if (
        std::filesystem::exists("assets/audio/hum.wav") &&
        ma_sound_init_from_file(
            Engine,
            "assets/audio/hum.wav",
            0,
            nullptr,
            nullptr,
            Hum
        ) == MA_SUCCESS
    )
    {
        ma_sound_set_looping(Hum, MA_TRUE);
        ma_sound_set_volume(Hum, 0.12f);
        ma_sound_start(Hum);
        HumReady = true;
    }

    if (
        std::filesystem::exists("assets/audio/static.wav") &&
        ma_sound_init_from_file(
            Engine,
            "assets/audio/static.wav",
            0,
            nullptr,
            nullptr,
            Static
        ) == MA_SUCCESS
    )
    {
        ma_sound_set_looping(Static, MA_TRUE);
        ma_sound_set_volume(Static, 0.035f);
        ma_sound_start(Static);
        StaticReady = true;
    }

    if (!HumReady)
    {
        delete Hum;
        Hum = nullptr;
    }

    if (!StaticReady)
    {
        delete Static;
        Static = nullptr;
    }

    Started = true;
    return true;
}

void AudioSystem::Shutdown()
{
    if (!Started)
        return;

    if (Hum)
    {
        ma_sound_uninit(Hum);
        delete Hum;
        Hum = nullptr;
    }

    if (Static)
    {
        ma_sound_uninit(Static);
        delete Static;
        Static = nullptr;
    }

    if (Engine)
    {
        ma_engine_uninit(Engine);
        delete Engine;
        Engine = nullptr;
    }

    Started = false;
}

void AudioSystem::Update(float EntityDistance)
{
    if (!Started)
        return;

    const float Distance =
        std::isfinite(EntityDistance)
            ? EntityDistance
            : 100.0f;

    const float Threat = std::clamp(
        1.0f - (Distance - 3.0f) / 22.0f,
        0.0f,
        1.0f
    );

    if (Hum)
        ma_sound_set_volume(
            Hum,
            0.12f + Threat * 0.045f
        );

    if (Static)
        ma_sound_set_volume(
            Static,
            0.035f + Threat * 0.22f
        );
}

void AudioSystem::PlayShift(bool DemonForm)
{
    if (!Started || !Engine)
        return;

    if (std::filesystem::exists("assets/audio/shift.wav"))
    {
        ma_engine_play_sound(
            Engine,
            "assets/audio/shift.wav",
            nullptr
        );
    }

    if (
        DemonForm &&
        std::filesystem::exists(
            "assets/audio/entity-laugh.ogg"
        )
    )
    {
        ma_engine_play_sound(
            Engine,
            "assets/audio/entity-laugh.ogg",
            nullptr
        );
    }
}

void AudioSystem::PlayDeath()
{
    if (!Started || !Engine)
        return;

    if (std::filesystem::exists("assets/audio/entity-death.ogg"))
    {
        ma_engine_play_sound(
            Engine,
            "assets/audio/entity-death.ogg",
            nullptr
        );
        return;
    }

    if (std::filesystem::exists("assets/audio/death.wav"))
    {
        ma_engine_play_sound(
            Engine,
            "assets/audio/death.wav",
            nullptr
        );
    }
}
