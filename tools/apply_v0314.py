from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT / path).read_text(encoding='utf-8')

def write(path, text):
    (ROOT / path).write_text(text, encoding='utf-8', newline='\n')

def replace_once(path, old, new):
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f'{path}: expected one match, found {count}: {old[:120]!r}')
    write(path, text.replace(old, new, 1))

# Revert V0.3.13 world/HUD visual guesses while keeping its input/menu-state fix.
subprocess.run([
    'git', 'checkout', '669ff60485aa11f0185d79c56900fd057795a445', '--',
    'src/Rendering/Renderer.cpp',
    'src/World/WorldGenerator.cpp'
], cwd=ROOT, check=True)

replace_once('CMakeLists.txt',
             'project(BackroomsOffical VERSION 0.3.13 LANGUAGES C CXX)',
             'project(BackroomsOffical VERSION 0.3.14 LANGUAGES C CXX)')
replace_once('src/Core/Version.h',
             'inline constexpr const char* Text = "0.3.13";',
             'inline constexpr const char* Text = "0.3.14";')

rc = read('src/Platform/Windows/Backrooms.rc')
for old, new in [
    ('FILEVERSION 0,3,13,0', 'FILEVERSION 0,3,14,0'),
    ('PRODUCTVERSION 0,3,13,0', 'PRODUCTVERSION 0,3,14,0'),
    ('VALUE "FileVersion", "0.3.13\\0"', 'VALUE "FileVersion", "0.3.14\\0"'),
    ('VALUE "ProductVersion", "0.3.13\\0"', 'VALUE "ProductVersion", "0.3.14\\0"'),
]:
    if rc.count(old) != 1:
        raise RuntimeError(f'Backrooms.rc mismatch: {old}')
    rc = rc.replace(old, new, 1)
write('src/Platform/Windows/Backrooms.rc', rc)

# Fix the converter so it reads selectors that actually exist in style.css instead of
# silently falling back to the invented yellow-theme defaults.
converter = read('tools/convert_web_ui.py')
replacements = [
    ('prop(base_css, "#StartScreen .MenuContent h1", "font-size", "clamp(66px, 10vw, 138px)")',
     'prop(base_css, ".MenuContent h1", "font-size", "clamp(66px, 11vw, 150px)")'),
    ('prop(base_css, "#StartScreen .MenuTop", "color", "rgba(43,38,16,0.58)")',
     'prop(base_css, ".MenuTop", "color", "rgba(232,224,165,0.46)")'),
    ('prop(base_css, "#StartScreen .MenuIndex", "color", "rgba(49,42,15,0.67)")',
     'prop(base_css, ".MenuIndex", "color", "rgba(240,231,162,0.62)")'),
    ('prop(base_css, "#StartScreen .MenuIndex", "border-color", "rgba(49,42,15,0.42)")',
     'prop(base_css, ".MenuIndex", "border-bottom", "rgba(236,226,151,0.42)")'),
    ('prop(base_css, "#StartScreen .MenuContent h1", "color", "#27230f")',
     'prop(base_css, ".MenuContent h1", "color", "#e2d993")'),
    ('prop(base_css, "#StartScreen .MenuContent p", "color", "rgba(43,37,13,0.72)")',
     'prop(base_css, ".MenuContent p", "color", "rgba(233,226,173,0.66)")'),
    ('prop(base_css, "#StartScreen #StartButton", "color", "#2a2510")',
     'prop(base_css, "#StartButton", "color", "#f3ebaf")'),
    ('prop(base_css, "#StartScreen #StartButton", "border-top-color", "rgba(45,39,14,0.56)")',
     'prop(base_css, "#StartButton", "border-top", "rgba(241,232,160,0.58)")'),
    ('prop(base_css, "#StartScreen #StartButton", "border-bottom-color", "rgba(45,39,14,0.25)")',
     'prop(base_css, "#StartButton", "border-bottom", "rgba(241,232,160,0.2)")'),
    ('prop(base_css, "#StartScreen #LoadStatus", "color", "rgba(45,39,14,0.48)")',
     'prop(base_css, "#LoadStatus", "color", "rgba(232,224,165,0.34)")'),
    ('background_color = rgba("#c8bb61")', 'background_color = rgba("#0b0a06")'),
    ("prop(base_css, '#StartScreen .MenuContent h1', 'letter-spacing', '-0.07em')",
     "prop(base_css, '.MenuContent h1', 'letter-spacing', '-0.075em')"),
    ("prop(base_css, '#StartScreen .MenuContent p', 'font-weight', '600')",
     "prop(base_css, '.MenuContent p', 'font-weight', '400')"),
]
for old, new in replacements:
    if old not in converter:
        raise RuntimeError(f'converter selector/default not found: {old}')
    converter = converter.replace(old, new, 1)
write('tools/convert_web_ui.py', converter)

# Replace the fake yellow menu background with the actual browser #StartScreen look.
menu = read('src/Rendering/MenuScreen.cpp')
start = menu.index('void Renderer::DrawMenuBackdrop()')
end = menu.index('void Renderer::DrawMenuText(', start)
real_backdrop = r'''void Renderer::DrawMenuBackdrop()
{
    // Direct native rendering of the original style.css StartScreen:
    // radial-gradient(circle at 72% 42%, rgba(213,200,104,.16), transparent 34%),
    // linear-gradient(115deg, #1c1a0e 0%, #0d0c08 56%, #070706 100%).
    const glm::vec3 Left{28.0f / 255.0f, 26.0f / 255.0f, 14.0f / 255.0f};
    const glm::vec3 Middle{13.0f / 255.0f, 12.0f / 255.0f, 8.0f / 255.0f};
    const glm::vec3 Right{7.0f / 255.0f, 7.0f / 255.0f, 6.0f / 255.0f};
    const glm::vec3 Glow{213.0f / 255.0f, 200.0f / 255.0f, 104.0f / 255.0f};

    const int W = static_cast<int>(Width);
    const int H = static_cast<int>(Height);
    constexpr int Band = 8;

    for (int X = 0; X < W; X += Band)
    {
        const float T = W > 1
            ? static_cast<float>(X) / static_cast<float>(W - 1)
            : 0.0f;

        glm::vec3 Base;
        if (T <= 0.56f)
        {
            const float Local = T / 0.56f;
            Base = Left * (1.0f - Local) + Middle * Local;
        }
        else
        {
            const float Local = (T - 0.56f) / 0.44f;
            Base = Middle * (1.0f - Local) + Right * Local;
        }

        DrawRect(X, 0, std::min(Band, W - X), H, Base);
    }

    const int CenterX = static_cast<int>(std::round(W * 0.72f));
    const int CenterY = static_cast<int>(std::round(H * 0.42f));
    const int RadiusX = std::max(1, static_cast<int>(std::round(W * 0.34f)));
    const int RadiusY = std::max(1, static_cast<int>(std::round(H * 0.34f)));

    for (int I = 10; I >= 1; --I)
    {
        const float T = static_cast<float>(I) / 10.0f;
        const float Alpha = 0.016f * (1.0f - T + 0.10f);
        const int RX = static_cast<int>(RadiusX * T);
        const int RY = static_cast<int>(RadiusY * T);
        const glm::vec3 Tint = Right * (1.0f - Alpha) + Glow * Alpha;
        DrawRect(CenterX - RX, CenterY - RY, RX * 2, RY * 2, Tint);
    }

    // Exact 98px repeating grid from #StartScreen::before.
    const glm::vec3 GridV = Right * 0.975f +
        glm::vec3{235.0f / 255.0f, 224.0f / 255.0f, 143.0f / 255.0f} * 0.025f;
    const glm::vec3 GridH = Right * 0.982f +
        glm::vec3{235.0f / 255.0f, 224.0f / 255.0f, 143.0f / 255.0f} * 0.018f;

    for (int X = 97; X < W; X += 98)
        DrawRect(X, 0, 1, H, GridV);

    for (int Y = 97; Y < H; Y += 98)
        DrawRect(0, Y, W, 1, GridH);

    // .MenuNoise: 1px line every 3px at the original tiny effective opacity.
    const float NoiseAlpha = 0.12f * 0.018f;
    const glm::vec3 NoiseColor = Right * (1.0f - NoiseAlpha) +
        glm::vec3{1.0f} * NoiseAlpha;

    for (int Y = 0; Y < H; Y += 3)
        DrawRect(0, Y, W, 1, NoiseColor);
}

'''
menu = menu[:start] + real_backdrop + menu[end:]
write('src/Rendering/MenuScreen.cpp', menu)

subprocess.run([
    'python', 'tools/convert_web_ui.py',
    'assets/ui/original/style.css',
    'src/Rendering/GeneratedWebUi.h'
], cwd=ROOT, check=True)

write('update/release_notes.txt',
      'V0.3.14 removes the invented yellow native menu theme and restores the actual original browser UI style from style.css: dark radial/linear StartScreen background, original yellow typography, real 98px grid/noise, exact 680px content width, 11vw/150px title sizing, original paragraph weight/colors, and the real bordered ENTER LEVEL 0 button. It also removes the added ESC MENU black HUD box and reverts the V0.3.13 world-lighting/material guesses back to the pre-change browser-matched values. The CSS converter selector bug was fixed so it no longer misses the real selectors and silently generates fallback colors/sizes.\n')

print('V0.3.14 real HTML style patch applied')
