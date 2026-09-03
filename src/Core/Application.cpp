#include "Application.h"
#include "Version.h"

#include <glad/gl.h>

#include <algorithm>
#include <cmath>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace
{
    std::filesystem::path ExecutableDirectory()
    {
#ifdef _WIN32
        std::wstring Buffer;
        Buffer.resize(32768);

        const DWORD Length = GetModuleFileNameW(
            nullptr,
            Buffer.data(),
            static_cast<DWORD>(Buffer.size())
        );

        if (Length > 0 && Length < Buffer.size())
        {
            Buffer.resize(Length);
            return std::filesystem::path(Buffer).parent_path();
        }
#endif

        return std::filesystem::current_path();
    }
}

Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    if (!InitializeWindow())
        return false;

    if (!InitializeOpenGL())
    {
        Shutdown();
        return false;
    }

    if (!Backrooms.Initialize(Width, Height))
    {
        Shutdown();
        return false;
    }

    SetMouseCaptured(false);

    Updater.Initialize(
        ExecutableDirectory(),
        BuildVersion::UpdateManifestUrl,
        BuildVersion::Text
    );
    Updater.BeginCheck();

    CounterFrequency =
        static_cast<double>(SDL_GetPerformanceFrequency());

    LastCounter = SDL_GetPerformanceCounter();

    Running = true;
    Initialized = true;

    return true;
}

bool Application::InitializeWindow()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_Log(
            "SDL initialization failed: %s",
            SDL_GetError()
        );
        return false;
    }

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MAJOR_VERSION,
        4
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_MINOR_VERSION,
        1
    );

    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE
    );

    SDL_GL_SetAttribute(
        SDL_GL_DOUBLEBUFFER,
        1
    );

    SDL_GL_SetAttribute(
        SDL_GL_DEPTH_SIZE,
        24
    );

    Window = SDL_CreateWindow(
        "Backrooms Offical",
        static_cast<int>(Width),
        static_cast<int>(Height),
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!Window)
    {
        SDL_Log(
            "Window creation failed: %s",
            SDL_GetError()
        );
        return false;
    }

    int PixelWidth = 0;
    int PixelHeight = 0;

    if (
        SDL_GetWindowSizeInPixels(
            Window,
            &PixelWidth,
            &PixelHeight
        )
    )
    {
        Width = static_cast<uint32_t>(
            std::max(PixelWidth, 1)
        );

        Height = static_cast<uint32_t>(
            std::max(PixelHeight, 1)
        );
    }

    return true;
}

bool Application::InitializeOpenGL()
{
    GLContext = SDL_GL_CreateContext(Window);

    if (!GLContext)
    {
        SDL_Log(
            "OpenGL context creation failed: %s",
            SDL_GetError()
        );
        return false;
    }

    if (
        gladLoadGL(
            reinterpret_cast<GLADloadfunc>(
                SDL_GL_GetProcAddress
            )
        ) == 0
    )
    {
        SDL_Log("GLAD failed to load OpenGL.");
        return false;
    }

    SDL_GL_SetSwapInterval(1);

    return true;
}

void Application::SetMouseCaptured(bool Captured)
{
    if (!Window)
        return;

    if (
        !SDL_SetWindowRelativeMouseMode(
            Window,
            Captured
        )
    )
    {
        SDL_Log(
            "Relative mouse mode failed: %s",
            SDL_GetError()
        );
        return;
    }

    MouseCaptured = Captured;
}

void Application::HandleResize()
{
    int PixelWidth = 0;
    int PixelHeight = 0;

    if (
        !SDL_GetWindowSizeInPixels(
            Window,
            &PixelWidth,
            &PixelHeight
        )
    )
    {
        return;
    }

    if (PixelWidth <= 0 || PixelHeight <= 0)
        return;

    Width = static_cast<uint32_t>(PixelWidth);
    Height = static_cast<uint32_t>(PixelHeight);

    Backrooms.Resize(Width, Height);
}

void Application::ProcessEvents()
{
    SDL_Event Event;

    while (SDL_PollEvent(&Event))
    {
        if (Event.type == SDL_EVENT_QUIT)
        {
            Running = false;
            continue;
        }

        if (
            Event.type ==
            SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
        )
        {
            HandleResize();
        }

        const bool UpdateActive =
            !UpdateBypassed &&
            Updater.ShouldBlockGame();

        if (UpdateActive)
        {
            if (MouseCaptured)
                SetMouseCaptured(false);

            const bool Activate =
                (
                    Event.type ==
                    SDL_EVENT_MOUSE_BUTTON_DOWN
                ) ||
                (
                    Event.type ==
                    SDL_EVENT_KEY_DOWN &&
                    !Event.key.repeat &&
                    Event.key.scancode ==
                        SDL_SCANCODE_RETURN
                );

            if (
                Updater.ReadyToApply() &&
                Activate
            )
            {
                if (Updater.LaunchApplyAndRestart())
                    Running = false;

                continue;
            }

            if (Updater.Failed())
            {
                if (Activate)
                {
                    Updater.BeginCheck();
                    continue;
                }

                if (
                    Event.type ==
                        SDL_EVENT_KEY_DOWN &&
                    !Event.key.repeat &&
                    Event.key.scancode ==
                        SDL_SCANCODE_ESCAPE
                )
                {
                    UpdateBypassed = true;
                    continue;
                }
            }

            continue;
        }

        if (
            Event.type == SDL_EVENT_KEY_DOWN &&
            !Event.key.repeat &&
            Event.key.scancode == SDL_SCANCODE_ESCAPE &&
            MouseCaptured
        )
        {
            SetMouseCaptured(false);
        }

        if (
            Event.type ==
            SDL_EVENT_MOUSE_BUTTON_DOWN &&
            !MouseCaptured
        )
        {
            SetMouseCaptured(true);
        }

        if (
            Event.type == SDL_EVENT_KEY_DOWN &&
            !Event.key.repeat &&
            !MouseCaptured &&
            (
                Event.key.scancode == SDL_SCANCODE_RETURN ||
                Event.key.scancode == SDL_SCANCODE_SPACE
            )
        )
        {
            SetMouseCaptured(true);
        }

        Backrooms.HandleEvent(
            Event,
            MouseCaptured
        );
    }
}

int Application::Run()
{
    while (Running)
    {
        const uint64_t CurrentCounter =
            SDL_GetPerformanceCounter();

        float DeltaTime =
            static_cast<float>(
                static_cast<double>(
                    CurrentCounter - LastCounter
                ) /
                CounterFrequency
            );

        LastCounter = CurrentCounter;

        DeltaTime = std::clamp(
            DeltaTime,
            0.0f,
            0.05f
        );

        TotalTime += DeltaTime;

        if (
            !UpdateBypassed &&
            Updater.HasUpdate()
        )
        {
            Updater.BeginDownload();
        }

        ProcessEvents();

        if (!Running)
            break;

        const bool UpdateActive =
            !UpdateBypassed &&
            Updater.ShouldBlockGame();

        if (UpdateActive)
        {
            Backrooms.RenderUpdateScreen(
                Updater.VisualState()
            );
        }
        else
        {
            Backrooms.Update(
                DeltaTime,
                MouseCaptured
            );

            Backrooms.Render(TotalTime);
        }

        SDL_SetWindowTitle(
            Window,
            Backrooms.WindowTitle().c_str()
        );

        SDL_GL_SwapWindow(Window);
    }

    Shutdown();

    return 0;
}

void Application::Shutdown()
{
    if (!Window && !GLContext)
        return;

    Updater.Shutdown();
    Backrooms.Shutdown();

    if (GLContext)
    {
        SDL_GL_DestroyContext(GLContext);
        GLContext = nullptr;
    }

    if (Window)
    {
        SDL_DestroyWindow(Window);
        Window = nullptr;
    }

    SDL_Quit();

    Running = false;
    Initialized = false;
}
