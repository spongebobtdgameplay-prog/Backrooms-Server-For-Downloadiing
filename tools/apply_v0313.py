from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8", newline="\n")


def replace_once(path, old, new):
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected 1 match, found {count}: {old[:120]!r}")
    write(path, text.replace(old, new, 1))


# Version bump.
replace_once("CMakeLists.txt", "project(BackroomsOffical VERSION 0.3.12 LANGUAGES C CXX)", "project(BackroomsOffical VERSION 0.3.13 LANGUAGES C CXX)")
replace_once("src/Core/Version.h", 'inline constexpr const char* Text = "0.3.12";', 'inline constexpr const char* Text = "0.3.13";')

rc = read("src/Platform/Windows/Backrooms.rc")
for old, new in [
    ("FILEVERSION 0,3,12,0", "FILEVERSION 0,3,13,0"),
    ("PRODUCTVERSION 0,3,12,0", "PRODUCTVERSION 0,3,13,0"),
    ('VALUE "FileVersion", "0.3.12\\0"', 'VALUE "FileVersion", "0.3.13\\0"'),
    ('VALUE "ProductVersion", "0.3.12\\0"', 'VALUE "ProductVersion", "0.3.13\\0"'),
]:
    if rc.count(old) != 1:
        raise RuntimeError(f"Backrooms.rc mismatch for {old!r}")
    rc = rc.replace(old, new, 1)
write("src/Platform/Windows/Backrooms.rc", rc)

# Stop a focus click from instantly skipping the start screen. Mouse clicks no longer
# act as global ENTER. Keyboard Enter/Space starts/resumes; the rendered button stays
# visible instead of being skipped by the click used to focus the game window.
replace_once(
    "src/Game/Game.cpp",
    '''    const bool Activate =\n        Event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||\n        (\n            KeyDown &&\n            (\n                Event.key.scancode == SDL_SCANCODE_RETURN ||\n                Event.key.scancode == SDL_SCANCODE_SPACE\n            )\n        );''',
    '''    const bool Activate =\n        KeyDown &&\n        (\n            Event.key.scancode == SDL_SCANCODE_RETURN ||\n            Event.key.scancode == SDL_SCANCODE_SPACE\n        );'''
)

# Gameplay readability: add a visible menu affordance and strengthen HUD contrast.
replace_once(
    "src/Rendering/Renderer.cpp",
    '''    DrawMenuText(\n        Version,\n        static_cast<int>(Width) - VersionWidth - 26,\n        40,\n        9,\n        400,\n        0.16f,\n        HtmlYellow,\n        0.34f,\n        false\n    );''',
    '''    DrawMenuText(\n        Version,\n        static_cast<int>(Width) - VersionWidth - 26,\n        40,\n        9,\n        400,\n        0.16f,\n        HtmlYellow,\n        0.48f,\n        false\n    );\n\n    const std::string MenuHint = "ESC  MENU";\n    const int MenuHintWidth = MenuTextWidth(MenuHint, 10, 700, 0.14f);\n    const int MenuHintX = static_cast<int>(Width) - MenuHintWidth - 26;\n    const int MenuHintY = 58;\n\n    DrawRect(\n        MenuHintX - 9,\n        MenuHintY - 6,\n        MenuHintWidth + 18,\n        24,\n        {0.055f, 0.050f, 0.028f}\n    );\n\n    DrawMenuText(\n        MenuHint,\n        MenuHintX,\n        MenuHintY,\n        10,\n        700,\n        0.14f,\n        HtmlYellow,\n        0.80f,\n        true\n    );'''
)

# Replace the blown-out lighting curve with a filmic-looking exposure/tone map and
# stronger surface separation. This keeps fluorescent hotspots without turning the
# entire wallpaper nearly white/yellow.
replace_once(
    "src/Rendering/Renderer.cpp",
    '''    vec3 AmbientLight =\n        vec3(1.0, 0.8879, 0.6240) * 0.46;\n\n    vec3 HemisphereLight = mix(\n        vec3(0.1221, 0.1144, 0.0802),\n        vec3(1.0, 0.9216, 0.7157),\n        UpFacing\n    ) * 0.34;''',
    '''    vec3 AmbientLight =\n        vec3(0.91, 0.80, 0.54) * 0.30;\n\n    vec3 HemisphereLight = mix(\n        vec3(0.095, 0.088, 0.060),\n        vec3(0.88, 0.80, 0.59),\n        UpFacing\n    ) * 0.22;'''
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''        vec3 LightColor = uLightColor[I].rgb * uLightColor[I].w;''',
    '''        vec3 LightColor = uLightColor[I].rgb * uLightColor[I].w * 0.82;'''
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''    float FogAmount = smoothstep(26.0, 72.0, DistanceToCamera);\n    vec3 FogColor = vec3(0.4735, 0.4342, 0.2582);\n\n    vec3 FinalColor = mix(Lighting, FogColor, FogAmount);\n    FinalColor = pow(max(FinalColor, vec3(0.0)), vec3(1.0 / 2.2));''',
    '''    float FogAmount = smoothstep(24.0, 64.0, DistanceToCamera);\n    vec3 FogColor = vec3(0.405, 0.365, 0.205);\n\n    vec3 FinalColor = mix(Lighting, FogColor, FogAmount);\n\n    // Soft shoulder: fluorescent panels can bloom bright, but wallpaper and carpet\n    // retain texture and color separation instead of clipping into a flat yellow.\n    FinalColor = vec3(1.0) - exp(-max(FinalColor, vec3(0.0)) * 0.88);\n    FinalColor = pow(FinalColor, vec3(1.0 / 2.2));'''
)

# Stronger wallpaper/carpet/ceiling material separation.
replace_once(
    "src/Rendering/Renderer.cpp",
    '''        Result *= mix(1.0, 0.955, DarkStripe);\n        Result *= mix(1.0, 1.075, BrightStripe);\n        Result *= mix(1.0, 0.975, MotifCenter * 0.5);\n        Result *= 0.97 + (Fiber - 0.5) * 0.07;''',
    '''        Result *= mix(1.0, 0.90, DarkStripe);\n        Result *= mix(1.0, 1.045, BrightStripe);\n        Result *= mix(1.0, 0.94, MotifCenter * 0.65);\n        Result *= 0.94 + (Fiber - 0.5) * 0.10;'''
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''        Result *= 0.84 + Fiber * 0.20;\n        Result *= 0.965 + Fine * 0.055;\n        Result *= 0.975 + Bands * 0.035;''',
    '''        Result *= 0.76 + Fiber * 0.22;\n        Result *= 0.93 + Fine * 0.07;\n        Result *= 0.95 + Bands * 0.05;'''
)
replace_once(
    "src/Rendering/Renderer.cpp",
    '''        Result *= 0.96 + Speckle * 0.07;\n        Result *= mix(1.0, 0.74, Grid * 0.48);''',
    '''        Result *= 0.90 + Speckle * 0.08;\n        Result *= mix(1.0, 0.66, Grid * 0.56);'''
)

# Rebalance base materials so the scene is no longer a uniform yellow wash.
replace_once("src/World/WorldGenerator.cpp", "{0.5984f, 0.4937f, 0.1029f}", "{0.515f, 0.420f, 0.086f}")
replace_once("src/World/WorldGenerator.cpp", "{0.1904f, 0.1611f, 0.0884f}", "{0.155f, 0.132f, 0.076f}")
replace_once("src/World/WorldGenerator.cpp", "{0.5595f, 0.5004f, 0.2800f}", "{0.430f, 0.394f, 0.235f}")
replace_once("src/World/WorldGenerator.cpp", "Light.BaseIntensity = 1.25f + Random() * 0.65f;", "Light.BaseIntensity = 0.95f + Random() * 0.48f;")

# Release note.
write(
    "update/release_notes.txt",
    "V0.3.13 fixes the start-screen skip caused by focus mouse clicks, keeps the real CSS-converted menu visible until Enter/Space is chosen, adds a clear ESC MENU control cue during gameplay, and reworks Level 0 rendering so wallpaper, carpet and ceiling keep visible texture instead of washing into flat yellow. Ambient/hemisphere light, fluorescent intensity, fog, material contrast and tone mapping were all rebalanced for stronger depth while preserving the original yellow Backrooms look.\n"
)

print("V0.3.13 patch applied")
