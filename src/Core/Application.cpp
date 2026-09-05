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

#ifdef _WIN32
    constexpr wchar_t TouchpadRegistryPath[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad";

    constexpr wchar_t TouchpadAapValue[] =
        L"AAPThreshold";

    struct TouchpadOverrideState
    {
        bool Active = false;
        bool RegistryActive = false;
        bool HadAapThreshold = false;
        DWORD SavedAapThreshold = 0;

#if defined(SPI_GETTOUCHPADPARAMETERS) && \
    defined(SPI_SETTOUCHPADPARAMETERS) && \
    defined(TOUCHPAD_PARAMETERS_VERSION_1)
        bool ApiActive = false;
        TOUCHPAD_PARAMETERS_V1 SavedTouchpadParameters{};
#endif
    };

    TouchpadOverrideState TouchpadOverride{};

    void NotifyTouchpadSettingChanged()
    {
        DWORD_PTR Ignored = 0;

        SendMessageTimeoutW(
  HWND_BROADCAST,
  WM_SETTINGCHANGE,
  0,
  reinterpret_cast<LPARAM>(TouchpadRegistryPath),
  SMTO_ABORTIFHUNG,
  150,
  &Ignored
        );
    }

    bool EnablePrecisionTouchpadAapOverride()
    {
        HKEY Key = nullptr;

        if (
  RegOpenKeyExW(
      HKEY_CURRENT_USER,
      TouchpadRegistryPath,
      0,
      KEY_QUERY_VALUE | KEY_SET_VALUE,
      &Key
  ) != ERROR_SUCCESS)
        {
  return false;
        }

        DWORD PreviousValue = 0;
        DWORD PreviousType = 0;
        DWORD PreviousSize = sizeof(PreviousValue);

        const LONG QueryResult =
  RegQueryValueExW(
      Key,
      TouchpadAapValue,
      nullptr,
      &PreviousType,
      reinterpret_cast<BYTE*>(&PreviousValue),
      &PreviousSize
  );

        if (QueryResult == ERROR_SUCCESS)
        {
  if (PreviousType != REG_DWORD)
  {
      RegCloseKey(Key);
      return false;
  }

  TouchpadOverride.HadAapThreshold = true;
  TouchpadOverride.SavedAapThreshold = PreviousValue;
        }
        else if (QueryResult == ERROR_FILE_NOT_FOUND)
        {
  TouchpadOverride.HadAapThreshold = false;
  TouchpadOverride.SavedAapThreshold = 0;
        }
        else
        {
  RegCloseKey(Key);
  return false;
        }

        const DWORD MaximumSensitivity = 0;

        const LONG SetResult =
  RegSetValueExW(
      Key,
      TouchpadAapValue,
      0,
      REG_DWORD,
      reinterpret_cast<const BYTE*>(&MaximumSensitivity),
      sizeof(MaximumSensitivity)
  );

        RegCloseKey(Key);

        if (SetResult != ERROR_SUCCESS)
  return false;

        TouchpadOverride.RegistryActive = true;
        NotifyTouchpadSettingChanged();
        return true;
    }

    void RestorePrecisionTouchpadAapOverride()
    {
        if (!TouchpadOverride.RegistryActive)
  return;

        HKEY Key = nullptr;

        if (
  RegOpenKeyExW(
      HKEY_CURRENT_USER,
      TouchpadRegistryPath,
      0,
      KEY_SET_VALUE,
      &Key
  ) == ERROR_SUCCESS)
        {
  if (TouchpadOverride.HadAapThreshold)
  {
      RegSetValueExW(
          Key,
          TouchpadAapValue,
          0,
          REG_DWORD,
          reinterpret_cast<const BYTE*>(
              &TouchpadOverride.SavedAapThreshold
          ),
          sizeof(TouchpadOverride.SavedAapThreshold)
      );
  }
  else
  {
      RegDeleteValueW(
          Key,
          TouchpadAapValue
      );
  }

  RegCloseKey(Key);
  NotifyTouchpadSettingChanged();
        }

        TouchpadOverride.RegistryActive = false;
    }

    void SetGameplayTouchpadMode(bool Enabled)
    {
        if (Enabled)
        {
  if (TouchpadOverride.Active)
      return;

  bool Changed =
      EnablePrecisionTouchpadAapOverride();

#if defined(SPI_GETTOUCHPADPARAMETERS) && \
    defined(SPI_SETTOUCHPADPARAMETERS) && \
    defined(TOUCHPAD_PARAMETERS_VERSION_1)
  TOUCHPAD_PARAMETERS_V1 Current{};
  Current.versionNumber =
      TOUCHPAD_PARAMETERS_VERSION_1;

  if (
      SystemParametersInfoW(
          SPI_GETTOUCHPADPARAMETERS,
          static_cast<UINT>(sizeof(Current)),
          &Current,
          0
      ) &&
      Current.touchpadPresent)
  {
      TouchpadOverride.SavedTouchpadParameters = Current;
      Current.sensitivityLevel =
          TOUCHPAD_SENSITIVITY_LEVEL_MOST_SENSITIVE;

      if (
          SystemParametersInfoW(
              SPI_SETTOUCHPADPARAMETERS,
              static_cast<UINT>(sizeof(Current)),
              &Current,
              SPIF_SENDCHANGE
          ))
      {
          TouchpadOverride.ApiActive = true;
          Changed = true;
      }
  }
#endif

  TouchpadOverride.Active = Changed;
  return;
        }

        RestorePrecisionTouchpadAapOverride();

#if defined(SPI_GETTOUCHPADPARAMETERS) && \
    defined(SPI_SETTOUCHPADPARAMETERS) && \
    defined(TOUCHPAD_PARAMETERS_VERSION_1)
        if (TouchpadOverride.ApiActive)
        {
  SystemParametersInfoW(
      SPI_SETTOUCHPADPARAMETERS,
      static_cast<UINT>(
          sizeof(TouchpadOverride.SavedTouchpadParameters)
      ),
      &TouchpadOverride.SavedTouchpadParameters,
      SPIF_SENDCHANGE
  );
        }
#endif

        TouchpadOverride = {};
    }
#else
    void SetGameplayTouchpadMode(bool Enabled)
    {
        static_cast<void>(Enabled);
    }
#endif
}

Application::~Application()
{
    Shutdown();
}

bool Application::Initialize()
{
    if (!InitializeWindow())
        return false;

    std::error_code WorkingDirectoryError;
    std::filesystem::current_path(
        ExecutableDirectory(),
        WorkingDirectoryError
    );

    if (WorkingDirectoryError)
    {
        SDL_Log(
            "Could not set working directory to executable folder: %s",
            WorkingDirectoryError.message().c_str()
        );
    }

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
#ifdef _WIN32
    // Raw GameInput keeps keyboard and pointing input independent.
    SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "1");
#endif

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

    SetGameplayTouchpadMode(Captured);

    MouseCaptured = Captured;
    Backrooms.OnMouseCaptureChanged(Captured);
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
            Updater.ShouldBlockGame();

        if (UpdateActive)
        {
            if (MouseCaptured)
                SetMouseCaptured(false);

            const bool Activate =
                Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                (
                    Event.type == SDL_EVENT_KEY_DOWN &&
                    !Event.key.repeat &&
                    Event.key.scancode == SDL_SCANCODE_RETURN
                );

            if (
                Updater.ReadyToInstall() &&
                Activate
            )
            {
                if (Updater.LaunchInstaller())
                    Running = false;

                continue;
            }

            if (Updater.Failed() && Activate)
            {
                Updater.BeginCheck();
                continue;
            }

            continue;
        }

        Backrooms.HandleEvent(
            Event,
            MouseCaptured
        );
    }

    const bool ShouldCapture =
        !Updater.ShouldBlockGame() &&
        Backrooms.ShouldCaptureMouse();

    if (ShouldCapture != MouseCaptured)
        SetMouseCaptured(ShouldCapture);
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

        ProcessEvents();

        if (!Running)
            break;

        if (Updater.HasUpdate())
            Updater.BeginDownload();

        const bool UpdateActive =
            Updater.ShouldBlockGame();

        if (UpdateActive)
        {
            Backrooms.RenderUpdateScreenV2(
                Updater.VisualState()
            );
        }
        else
        {
            Backrooms.Update(
                DeltaTime,
                MouseCaptured
            );

            if (Backrooms.HasMenuOverlay())
            {
                Backrooms.RenderMenuOverlay();
            }
            else
            {
                Backrooms.Render(TotalTime);
            }
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

    SetGameplayTouchpadMode(false);

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
