#pragma once

#include "../Game/Game.h"
#include "../Updater/UpdaterService.h"

#include <SDL3/SDL.h>

#include <cstdint>

class Application
{
public:
    Application() = default;
    ~Application();

    bool Initialize();
    int Run();

private:
    bool InitializeWindow();
    bool InitializeOpenGL();

    void ProcessEvents();
    void HandleResize();
    void SetMouseCaptured(bool Captured);
    void Shutdown();

    SDL_Window* Window = nullptr;
    SDL_GLContext GLContext = nullptr;

    Game Backrooms;
    UpdaterService Updater;

    bool Running = false;
    bool MouseCaptured = false;
    bool Initialized = false;

    uint32_t Width = 1600;
    uint32_t Height = 900;

    uint64_t LastCounter = 0;
    double CounterFrequency = 1.0;
    float TotalTime = 0.0f;
};
