#pragma once

#include <string>

struct ma_engine;
struct ma_sound;

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    bool Initialize();
    void Shutdown();

    void Update(float EntityDistance);
    void PlayShift(bool DemonForm);
    void PlayDeath();

private:
    ma_engine* Engine = nullptr;
    ma_sound* Hum = nullptr;
    ma_sound* Static = nullptr;

    bool Started = false;
};
