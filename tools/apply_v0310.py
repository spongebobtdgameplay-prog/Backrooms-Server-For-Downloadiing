from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8", newline="\n")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected exactly one match, found {count}: {old[:100]!r}")
    write(path, text.replace(old, new, 1))


# ---------------------------------------------------------------------------
# Version + Windows GUI subsystem (no console/Terminal window for the game).
# ---------------------------------------------------------------------------
replace_once(
    "CMakeLists.txt",
    "project(BackroomsOffical VERSION 0.3.9 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.10 LANGUAGES C CXX)",
)
replace_once(
    "CMakeLists.txt",
    "        WIN32_EXECUTABLE FALSE\n        OUTPUT_NAME \"Backrooms Offical\"",
    "        WIN32_EXECUTABLE TRUE\n        OUTPUT_NAME \"Backrooms Offical\"",
)
replace_once(
    "src/Core/Version.h",
    'inline constexpr const char* Text = "0.3.9";',
    'inline constexpr const char* Text = "0.3.10";',
)

rc = read("src/Platform/Windows/Backrooms.rc")
for old, new in [
    ("FILEVERSION 0,3,9,0", "FILEVERSION 0,3,10,0"),
    ("PRODUCTVERSION 0,3,9,0", "PRODUCTVERSION 0,3,10,0"),
    ('VALUE "FileVersion", "0.3.9\\0"', 'VALUE "FileVersion", "0.3.10\\0"'),
    ('VALUE "ProductVersion", "0.3.9\\0"', 'VALUE "ProductVersion", "0.3.10\\0"'),
]:
    if rc.count(old) != 1:
        raise RuntimeError(f"Backrooms.rc expected one {old!r}, found {rc.count(old)}")
    rc = rc.replace(old, new, 1)
write("src/Platform/Windows/Backrooms.rc", rc)


# ---------------------------------------------------------------------------
# Windows touchpad look fix.
# - Ask SDL to use GameInput for raw keyboard/mouse where available.
# - While gameplay owns relative mouse mode, temporarily set a Precision
#   Touchpad to MOST_SENSITIVE so Windows does not suppress touchpad mouse
#   generation after keyboard activity (the W + touchpad bug).
# - Restore the user's exact setting whenever capture stops or the app exits.
# ---------------------------------------------------------------------------
replace_once(
    "src/Core/Application.cpp",
    "        return std::filesystem::current_path();\n    }\n}",
    r'''        return std::filesystem::current_path();
    }

#ifdef _WIN32
#if defined(SPI_GETTOUCHPADPARAMETERS) && \
    defined(SPI_SETTOUCHPADPARAMETERS) && \
    defined(TOUCHPAD_PARAMETERS_VERSION_1)
    TOUCHPAD_PARAMETERS_V1 SavedTouchpadParameters{};
    bool HasSavedTouchpadParameters = false;
    bool GameplayTouchpadOverrideActive = false;

    void SetGameplayTouchpadMode(bool Enabled)
    {
        if (Enabled)
        {
            if (GameplayTouchpadOverrideActive)
                return;

            TOUCHPAD_PARAMETERS_V1 Current{};
            Current.versionNumber = TOUCHPAD_PARAMETERS_VERSION_1;

            if (!SystemParametersInfoW(
                    SPI_GETTOUCHPADPARAMETERS,
                    static_cast<UINT>(sizeof(Current)),
                    &Current,
                    0))
            {
                return;
            }

            if (!Current.touchpadPresent)
                return;

            SavedTouchpadParameters = Current;
            HasSavedTouchpadParameters = true;

            Current.sensitivityLevel =
                TOUCHPAD_SENSITIVITY_LEVEL_MOST_SENSITIVE;

            if (SystemParametersInfoW(
                    SPI_SETTOUCHPADPARAMETERS,
                    static_cast<UINT>(sizeof(Current)),
                    &Current,
                    0))
            {
                GameplayTouchpadOverrideActive = true;
            }
            else
            {
                HasSavedTouchpadParameters = false;
            }
        }
        else
        {
            if (
                GameplayTouchpadOverrideActive &&
                HasSavedTouchpadParameters)
            {
                SystemParametersInfoW(
                    SPI_SETTOUCHPADPARAMETERS,
                    static_cast<UINT>(
                        sizeof(SavedTouchpadParameters)
                    ),
                    &SavedTouchpadParameters,
                    0
                );
            }

            GameplayTouchpadOverrideActive = false;
            HasSavedTouchpadParameters = false;
        }
    }
#else
    void SetGameplayTouchpadMode(bool Enabled)
    {
        static_cast<void>(Enabled);
    }
#endif
#else
    void SetGameplayTouchpadMode(bool Enabled)
    {
        static_cast<void>(Enabled);
    }
#endif
}''',
)
replace_once(
    "src/Core/Application.cpp",
    "bool Application::InitializeWindow()\n{\n    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))",
    "bool Application::InitializeWindow()\n{\n#ifdef _WIN32\n    // Raw GameInput keeps keyboard and pointing input independent.\n    SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, \"1\");\n#endif\n\n    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))",
)
replace_once(
    "src/Core/Application.cpp",
    "    MouseCaptured = Captured;\n    Backrooms.OnMouseCaptureChanged(Captured);",
    "    SetGameplayTouchpadMode(Captured);\n\n    MouseCaptured = Captured;\n    Backrooms.OnMouseCaptureChanged(Captured);",
)
replace_once(
    "src/Core/Application.cpp",
    "    Updater.Shutdown();\n    Backrooms.Shutdown();",
    "    SetGameplayTouchpadMode(false);\n\n    Updater.Shutdown();\n    Backrooms.Shutdown();",
)


# ---------------------------------------------------------------------------
# Restore the stronger original browser camera motion instead of the reduced
# native bob amplitudes.
# ---------------------------------------------------------------------------
replace_once(
    "src/Player/Player.cpp",
    "                (WantsSprint ? 0.018f : 0.012f)",
    "                (WantsSprint ? 0.026f : 0.018f)",
)


# ---------------------------------------------------------------------------
# Restore the original 0.42 s ghost/demon squash-grow shapeshift on the real
# GLB models. The native entity already tracked ShiftProgress, but the real
# model renderer ignored it and swapped forms instantly.
# ---------------------------------------------------------------------------
replace_once(
    "src/Entity/Entity.h",
    "    bool IsDemonForm() const { return DemonForm; }\n    const glm::vec3& Position() const { return EntityPosition; }",
    "    bool IsDemonForm() const { return DemonForm; }\n    bool PreviousWasDemonForm() const { return PreviousDemonForm; }\n    float ShiftAmount() const { return ShiftProgress; }\n    const glm::vec3& Position() const { return EntityPosition; }",
)
replace_once(
    "src/Entity/Entity.h",
    "    bool Active = false;\n    bool DemonForm = false;\n    bool ShiftedThisFrame = false;",
    "    bool Active = false;\n    bool DemonForm = false;\n    bool PreviousDemonForm = false;\n    bool ShiftedThisFrame = false;",
)
replace_once(
    "src/Entity/Entity.cpp",
    "    Active = false;\n    DemonForm = false;\n    ShiftedThisFrame = false;",
    "    Active = false;\n    DemonForm = false;\n    PreviousDemonForm = false;\n    ShiftedThisFrame = false;",
)
replace_once(
    "src/Entity/Entity.cpp",
    "    if (DesiredDemon != DemonForm)\n    {\n        DemonForm = DesiredDemon;",
    "    if (DesiredDemon != DemonForm)\n    {\n        PreviousDemonForm = DemonForm;\n        DemonForm = DesiredDemon;",
)

replace_once(
    "src/Rendering/EntityModel.h",
    "        const glm::vec3& Position,\n        const glm::vec3& Forward,\n        const std::array<glm::vec4, 8>& LightPositions,",
    "        const glm::vec3& Position,\n        const glm::vec3& Forward,\n        const glm::vec3& VisualScale,\n        const std::array<glm::vec4, 8>& LightPositions,",
)
replace_once(
    "src/Rendering/EntityModel.cpp",
    "    const glm::vec3& Position,\n    const glm::vec3& Forward,\n    const std::array<glm::vec4, 8>& LightPositions,",
    "    const glm::vec3& Position,\n    const glm::vec3& Forward,\n    const glm::vec3& VisualScale,\n    const std::array<glm::vec4, 8>& LightPositions,",
)
replace_once(
    "src/Rendering/EntityModel.cpp",
    "        glm::rotate(\n            glm::mat4(1.0f),\n            Yaw,\n            glm::vec3{0.0f, 1.0f, 0.0f}\n        ) *\n        glm::scale(\n            glm::mat4(1.0f),\n            glm::vec3{Scale}\n        ) *",
    "        glm::rotate(\n            glm::mat4(1.0f),\n            Yaw,\n            glm::vec3{0.0f, 1.0f, 0.0f}\n        ) *\n        glm::scale(\n            glm::mat4(1.0f),\n            VisualScale\n        ) *\n        glm::scale(\n            glm::mat4(1.0f),\n            glm::vec3{Scale}\n        ) *",
)

replace_once(
    "src/Rendering/Renderer.h",
    "        const glm::vec3& Forward,\n        bool DemonForm\n    );",
    "        const glm::vec3& Forward,\n        bool DemonForm,\n        bool PreviousDemonForm,\n        float ShiftProgress\n    );",
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''void Renderer::DrawEntity(
    const glm::vec3& Position,
    const glm::vec3& Forward,
    bool DemonForm
)
{
    const EntityModel* Model =
        DemonForm
            ? &DemonEntityModel
            : &GhostEntityModel;

    if (!Model->IsReady())
    {
        Model =
            DemonForm
                ? &GhostEntityModel
                : &DemonEntityModel;
    }

    if (!Model->IsReady())
        return;

    Model->Draw(
        View,
        Projection,
        CameraPosition,
        Position,
        Forward,
        ActiveLightPositions,
        ActiveLightColors,
        ActiveLightCount
    );
}''',
    '''void Renderer::DrawEntity(
    const glm::vec3& Position,
    const glm::vec3& Forward,
    bool DemonForm,
    bool PreviousDemonForm,
    float ShiftProgress
)
{
    auto ResolveModel = [&](bool UseDemon) -> const EntityModel*
    {
        const EntityModel* Model =
            UseDemon
                ? &DemonEntityModel
                : &GhostEntityModel;

        if (!Model->IsReady())
        {
            Model =
                UseDemon
                    ? &GhostEntityModel
                    : &DemonEntityModel;
        }

        return Model->IsReady() ? Model : nullptr;
    };

    auto DrawForm = [&](bool UseDemon, const glm::vec3& VisualScale)
    {
        const EntityModel* Model = ResolveModel(UseDemon);

        if (Model == nullptr)
            return;

        Model->Draw(
            View,
            Projection,
            CameraPosition,
            Position,
            Forward,
            VisualScale,
            ActiveLightPositions,
            ActiveLightColors,
            ActiveLightCount
        );
    };

    const float Progress =
        std::clamp(ShiftProgress, 0.0f, 1.0f);

    if (
        Progress < 1.0f &&
        PreviousDemonForm != DemonForm)
    {
        const float Ease =
            Progress * Progress *
            (3.0f - 2.0f * Progress);

        const glm::vec3 PreviousScale{
            1.0f + Ease * 0.22f,
            std::max(0.02f, 1.0f - Ease),
            1.0f + Ease * 0.22f
        };

        const glm::vec3 CurrentScale{
            0.76f + Ease * 0.24f,
            0.48f + Ease * 0.52f,
            0.76f + Ease * 0.24f
        };

        DrawForm(PreviousDemonForm, PreviousScale);
        DrawForm(DemonForm, CurrentScale);
        return;
    }

    DrawForm(DemonForm, glm::vec3{1.0f});
}''',
)
replace_once(
    "src/Game/Game.cpp",
    "            Hunter.Forward(),\n            Hunter.IsDemonForm()\n        );",
    "            Hunter.Forward(),\n            Hunter.IsDemonForm(),\n            Hunter.PreviousWasDemonForm(),\n            Hunter.ShiftAmount()\n        );",
)


# ---------------------------------------------------------------------------
# Actually wire the smooth text renderer into every text path. V0.3.9 added
# SmoothTextRenderer but left DrawText/TextWidth using the 3x5 arcade glyphs,
# so menus, HUD, pause, updater and end screens could still look pixelated.
# ---------------------------------------------------------------------------
replace_once(
    "src/Rendering/Renderer.h",
    "    int TextWidth(const std::string& Text, int Scale) const;",
    "    int TextWidth(const std::string& Text, int Scale);",
)
replace_once(
    "src/Rendering/Renderer.cpp",
    "    if (!CreateShadowResources())\n        return false;\n\n    GhostEntityModel.Load(",
    "    if (!CreateShadowResources())\n        return false;\n\n    MenuTextRenderer.Initialize();\n    MenuTextRenderer.Resize(Width, Height);\n\n    GhostEntityModel.Load(",
)
replace_once(
    "src/Rendering/Renderer.cpp",
    "void Renderer::Shutdown()\n{\n    GhostEntityModel.Shutdown();",
    "void Renderer::Shutdown()\n{\n    MenuTextRenderer.Shutdown();\n\n    GhostEntityModel.Shutdown();",
)
replace_once(
    "src/Rendering/Renderer.cpp",
    "    Height = std::max(NewHeight, 1u);\n\n    glViewport(",
    "    Height = std::max(NewHeight, 1u);\n    MenuTextRenderer.Resize(Width, Height);\n\n    glViewport(",
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''int Renderer::TextWidth(const std::string& Text, int Scale) const
{
    if (Text.empty())
        return 0;

    return static_cast<int>(Text.size()) * 4 * Scale - Scale;
}''',
    '''int Renderer::TextWidth(const std::string& Text, int Scale)
{
    if (Text.empty() || Scale <= 0)
        return 0;

    if (MenuTextRenderer.IsReady())
    {
        const int PixelHeight = std::max(10, Scale * 8);
        const int Weight =
            Scale >= 6 ? 900 :
            Scale >= 3 ? 750 : 600;
        const float Tracking =
            Scale >= 6 ? -0.04f :
            Scale >= 3 ? 0.035f : 0.10f;

        const int WidthResult = MenuTextRenderer.Measure(
            Text,
            PixelHeight,
            Weight,
            Tracking
        );

        if (WidthResult > 0)
            return WidthResult;
    }

    return static_cast<int>(Text.size()) * 4 * Scale - Scale;
}''',
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''void Renderer::DrawText(
    const std::string& Text,
    int X,
    int Y,
    int Scale,
    const glm::vec3& Color
)
{
    if (Scale <= 0)
        return;

    glEnable(GL_SCISSOR_TEST);''',
    '''void Renderer::DrawText(
    const std::string& Text,
    int X,
    int Y,
    int Scale,
    const glm::vec3& Color
)
{
    if (Scale <= 0 || Text.empty())
        return;

    if (MenuTextRenderer.IsReady())
    {
        const int PixelHeight = std::max(10, Scale * 8);
        const int Weight =
            Scale >= 6 ? 900 :
            Scale >= 3 ? 750 : 600;
        const float Tracking =
            Scale >= 6 ? -0.04f :
            Scale >= 3 ? 0.035f : 0.10f;

        MenuTextRenderer.Draw(
            Text,
            X,
            Y,
            PixelHeight,
            Weight,
            Tracking,
            Color
        );
        return;
    }

    glEnable(GL_SCISSOR_TEST);''',
)
replace_once(
    "src/Rendering/Renderer.cpp",
    "        const auto Rows = GlyphRows(C);",
    "        const auto Rows = GlyphRows(\n            static_cast<char>(\n                std::toupper(\n                    static_cast<unsigned char>(C)\n                )\n            )\n        );",
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '            : "ENTER  ENTER LEVEL 0";',
    '            : "ENTER LEVEL 0";',
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '        "MONO YELLOW ROOMS DAMP CARPET",',
    '        "Mono-yellow rooms, damp carpet and fluorescent light",',
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '        "RESTORE THREE BREAKERS AND FIND THE POWERED EXIT",',
    '        "Restore three breakers and find the powered exit",',
)


write(
    "update/release_notes.txt",
    "V0.3.10 removes the Windows console window during normal play, fixes W + touchpad camera look on supported Windows Precision Touchpads while restoring the user's setting outside gameplay, enables SDL GameInput raw mouse/keyboard handling, wires smooth antialiased text through menus/HUD/pause/updater/end screens instead of the leftover arcade glyph renderer, fixes the duplicate ENTER menu label, restores the stronger original head-bob motion, and restores the old 0.42-second ghost/demon squash-grow shapeshift transition on the real entity models.\n",
)

print("V0.3.10 patch applied successfully.")
