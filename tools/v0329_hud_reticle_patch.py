from pathlib import Path
import re


def read(path):
    return Path(path).read_text(encoding="utf-8")


def write(path, text):
    Path(path).write_text(text, encoding="utf-8", newline="\n")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


renderer_path = "src/Rendering/Renderer.cpp"
renderer = read(renderer_path)
old_crosshair = '''void Renderer::DrawCrosshair()
{
    const GLint CenterX = static_cast<GLint>(Width / 2);
    const GLint CenterY = static_cast<GLint>(Height / 2);

    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.74f, 0.72f, 0.58f, 1.0f);

    glScissor(CenterX, CenterY - 5, 1, 10);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(CenterX - 5, CenterY, 10, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0.96f, 0.94f, 0.76f, 1.0f);
    glScissor(CenterX, CenterY, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);
}
'''
new_crosshair = '''void Renderer::DrawCrosshair()
{
    const int CenterX = static_cast<int>(Width / 2);
    const int CenterY = static_cast<int>(Height / 2);

    const glm::vec3 Stroke{
        0.055f,
        0.050f,
        0.032f
    };

    const glm::vec3 Reticle{
        0.98f,
        0.955f,
        0.78f
    };

    DrawRect(CenterX - 12, CenterY - 2, 7, 3, Stroke);
    DrawRect(CenterX + 5, CenterY - 2, 7, 3, Stroke);
    DrawRect(CenterX - 2, CenterY - 12, 3, 7, Stroke);
    DrawRect(CenterX - 2, CenterY + 5, 3, 7, Stroke);

    DrawRect(CenterX - 11, CenterY - 1, 5, 1, Reticle);
    DrawRect(CenterX + 6, CenterY - 1, 5, 1, Reticle);
    DrawRect(CenterX - 1, CenterY - 11, 1, 5, Reticle);
    DrawRect(CenterX - 1, CenterY + 6, 1, 5, Reticle);

    DrawRect(CenterX - 2, CenterY - 2, 3, 3, Stroke);
    DrawRect(CenterX - 1, CenterY - 1, 1, 1, Reticle);
}
'''
renderer = replace_once(renderer, old_crosshair, new_crosshair, "crosshair")
write(renderer_path, renderer)

text_renderer_path = "src/Rendering/GameTextRenderer.cpp"
text_renderer = read(text_renderer_path)
old_shadow = '''    if (Shadow)
    {
        DrawPass(
            X + 2,
            Y + 3,
            {0.0f, 0.0f, 0.0f},
            Opacity * 0.78f
        );

        DrawPass(
            X - 1,
            Y + 1,
            {0.0f, 0.0f, 0.0f},
            Opacity * 0.35f
        );
    }
'''
new_shadow = '''    if (Shadow)
    {
        const glm::vec3 StrokeColor{
            0.020f,
            0.018f,
            0.012f
        };

        const int StrokeRadius =
            PixelHeight >= 12 ? 2 : 1;

        DrawPass(
            X + 2,
            Y + StrokeRadius + 2,
            StrokeColor,
            Opacity * 0.48f
        );

        for (int OffsetY = -StrokeRadius; OffsetY <= StrokeRadius; OffsetY += StrokeRadius)
        {
            for (int OffsetX = -StrokeRadius; OffsetX <= StrokeRadius; OffsetX += StrokeRadius)
            {
                if (OffsetX == 0 && OffsetY == 0)
                    continue;

                DrawPass(
                    X + OffsetX,
                    Y + OffsetY,
                    StrokeColor,
                    Opacity * 0.92f
                );
            }
        }
    }
'''
text_renderer = replace_once(text_renderer, old_shadow, new_shadow, "game text outline")
write(text_renderer_path, text_renderer)

interactive_path = "src/Rendering/InteractiveMenu.cpp"
interactive = read(interactive_path)
start = interactive.find("void Renderer::DrawGameplayOverlayV3(")
if start < 0:
    raise RuntimeError("gameplay overlay start not found")
body_start = interactive.find("{", start)
if body_start < 0:
    raise RuntimeError("gameplay overlay body not found")
index = body_start
level = 0
end = -1
while index < len(interactive):
    char = interactive[index]
    if char == "{":
        level += 1
    elif char == "}":
        level -= 1
        if level == 0:
            end = index + 1
            break
    index += 1
if end < 0:
    raise RuntimeError("gameplay overlay end not found")
body = interactive[start:end]
pattern = re.compile(r"(GameplayTextRenderer\.Draw\([\s\S]*?\n\s*)false(\n\s*\);)")
body, count = pattern.subn(lambda match: match.group(1) + "true" + match.group(2), body)
if count != 6:
    raise RuntimeError(f"expected 6 gameplay HUD text draws, changed {count}")
interactive = interactive[:start] + body + interactive[end:]
write(interactive_path, interactive)

cmake_path = "CMakeLists.txt"
cmake = read(cmake_path)
cmake = replace_once(
    cmake,
    "project(BackroomsOffical VERSION 0.3.28 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.29 LANGUAGES C CXX)",
    "cmake version",
)
write(cmake_path, cmake)

version_path = "src/Core/Version.h"
version = read(version_path)
version = replace_once(
    version,
    'inline constexpr const char* Text = "0.3.28";',
    'inline constexpr const char* Text = "0.3.29";',
    "build version",
)
write(version_path, version)

rc_path = "src/Platform/Windows/Backrooms.rc"
rc = read(rc_path)
rc = rc.replace("FILEVERSION 0,3,28,0", "FILEVERSION 0,3,29,0")
rc = rc.replace("PRODUCTVERSION 0,3,28,0", "PRODUCTVERSION 0,3,29,0")
rc = rc.replace('VALUE "FileVersion", "0.3.28\\0"', 'VALUE "FileVersion", "0.3.29\\0"')
rc = rc.replace('VALUE "ProductVersion", "0.3.28\\0"', 'VALUE "ProductVersion", "0.3.29\\0"')
if "0.3.28" in rc or "0,3,28,0" in rc:
    raise RuntimeError("old rc version remains")
write(rc_path, rc)

notes_path = "update/release_notes.txt"
notes = (
    "V0.3.29 fixes the gameplay HUD readability and the center reticle. "
    "Gameplay labels now use a real dark multi-direction glyph stroke plus a subtle drop shadow, "
    "so LEVEL 0, RESTORE POWER, FPS, BUILD, SPRINT and interaction prompts stay readable over the bright Level 0 lighting. "
    "The old one-pixel connected plus crosshair has been removed and replaced with a custom separated four-tick reticle, "
    "a tiny center point and dark backing strokes so it reads as an intentional game reticle instead of a Windows/debug cursor.\n"
)
write(notes_path, notes)
