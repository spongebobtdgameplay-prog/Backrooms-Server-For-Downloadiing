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
        raise RuntimeError(f"{path}: expected exactly one match, found {count}: {old[:120]!r}")
    write(path, text.replace(old, new, 1))


# Version bump.
replace_once("CMakeLists.txt", "project(BackroomsOffical VERSION 0.3.10 LANGUAGES C CXX)", "project(BackroomsOffical VERSION 0.3.11 LANGUAGES C CXX)")
replace_once("src/Core/Version.h", 'inline constexpr const char* Text = "0.3.10";', 'inline constexpr const char* Text = "0.3.11";')

rc = read("src/Platform/Windows/Backrooms.rc")
for old, new in [
    ("FILEVERSION 0,3,10,0", "FILEVERSION 0,3,11,0"),
    ("PRODUCTVERSION 0,3,10,0", "PRODUCTVERSION 0,3,11,0"),
    ('VALUE "FileVersion", "0.3.10\\0"', 'VALUE "FileVersion", "0.3.11\\0"'),
    ('VALUE "ProductVersion", "0.3.10\\0"', 'VALUE "ProductVersion", "0.3.11\\0"'),
]:
    if rc.count(old) != 1:
        raise RuntimeError(f"Backrooms.rc expected one {old!r}, found {rc.count(old)}")
    rc = rc.replace(old, new, 1)
write("src/Platform/Windows/Backrooms.rc", rc)


# ---------------------------------------------------------------------------
# SmoothTextRenderer: reproduce the old HTML/CSS font stack and styling.
# The browser did NOT bundle a custom font file; style.css inherited:
#   "Arial Narrow", "Helvetica Neue", Arial, sans-serif
# We now resolve that stack on Windows instead of blindly requesting one face.
# Also add CSS-like opacity + shadow and better ClearType coverage.
# ---------------------------------------------------------------------------
replace_once(
    "src/Rendering/SmoothTextRenderer.h",
    '''    void Draw(
        const std::string& Text,
        int X,
        int Y,
        int PixelHeight,
        int Weight,
        float TrackingEm,
        const glm::vec3& Color
    );''',
    '''    void Draw(
        const std::string& Text,
        int X,
        int Y,
        int PixelHeight,
        int Weight,
        float TrackingEm,
        const glm::vec3& Color,
        float Opacity = 1.0f,
        bool Shadow = false
    );'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.h",
    "    GLint ColorLocation = -1;\n    GLint TextureLocation = -1;",
    "    GLint ColorLocation = -1;\n    GLint OpacityLocation = -1;\n    GLint TextureLocation = -1;"
)

replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    "#include <cmath>\n#include <sstream>",
    "#include <cmath>\n#include <cwchar>\n#include <sstream>"
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''uniform sampler2D uTexture;
uniform vec3 uColor;
out vec4 FragColor;
void main()
{
    float Alpha = texture(uTexture, vUv).r;
    FragColor = vec4(uColor, Alpha);
}''',
    '''uniform sampler2D uTexture;
uniform vec3 uColor;
uniform float uOpacity;
out vec4 FragColor;
void main()
{
    float Alpha = texture(uTexture, vUv).r;
    FragColor = vec4(uColor, Alpha * uOpacity);
}'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''    GLuint Compile(GLenum Type, const char* Source)
    {
        const GLuint Shader = glCreateShader(Type);''',
    '''#ifdef _WIN32
    int CALLBACK FontEnumerationCallback(
        const LOGFONTW*,
        const TEXTMETRICW*,
        DWORD,
        LPARAM Parameter
    )
    {
        bool* Found = reinterpret_cast<bool*>(Parameter);
        *Found = true;
        return 0;
    }

    bool HasFontFace(HDC DeviceContext, const wchar_t* Face)
    {
        LOGFONTW Font{};
        Font.lfCharSet = DEFAULT_CHARSET;
        wcsncpy_s(Font.lfFaceName, Face, _TRUNCATE);

        bool Found = false;
        EnumFontFamiliesExW(
            DeviceContext,
            &Font,
            FontEnumerationCallback,
            reinterpret_cast<LPARAM>(&Found),
            0
        );
        return Found;
    }

    const wchar_t* ResolveCssFontFace(HDC DeviceContext)
    {
        static constexpr const wchar_t* FontStack[] = {
            L"Arial Narrow",
            L"Helvetica Neue",
            L"Arial",
            L"Segoe UI"
        };

        for (const wchar_t* Face : FontStack)
        {
            if (HasFontFace(DeviceContext, Face))
                return Face;
        }

        return L"Arial";
    }
#endif

    GLuint Compile(GLenum Type, const char* Source)
    {
        const GLuint Shader = glCreateShader(Type);'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''    ColorLocation = glGetUniformLocation(Program, "uColor");
    TextureLocation = glGetUniformLocation(Program, "uTexture");''',
    '''    ColorLocation = glGetUniformLocation(Program, "uColor");
    OpacityLocation = glGetUniformLocation(Program, "uOpacity");
    TextureLocation = glGetUniformLocation(Program, "uTexture");'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''    HFONT Font = CreateFontW(
        -PixelHeight,
        0,
        0,
        0,
        std::clamp(Weight, 100, 900),
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Arial Narrow"
    );''',
    '''    const wchar_t* CssFace =
        ResolveCssFontFace(DeviceContext);

    HFONT Font = CreateFontW(
        -PixelHeight,
        0,
        0,
        0,
        std::clamp(Weight, 100, 900),
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_NATURAL_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        CssFace
    );'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''    const int BitmapWidth =
        std::max(Size.cx + 8, 1L);

    const int BitmapHeight =
        std::max(
            static_cast<int>(Metrics.tmHeight) + 8,
            1
        );''',
    '''    const int BitmapWidth =
        std::max(Size.cx + 12, 1L);

    const int BitmapHeight =
        std::max(
            static_cast<int>(Metrics.tmHeight) +
                static_cast<int>(Metrics.tmExternalLeading) +
                12,
            1
        );'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''    TextOutW(
        DeviceContext,
        4,
        2,
        Wide.c_str(),''',
    '''    TextOutW(
        DeviceContext,
        6,
        3,
        Wide.c_str(),'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    '''            Alpha[
                static_cast<std::size_t>(Y) *
                static_cast<std::size_t>(BitmapWidth) +
                static_cast<std::size_t>(X)
            ] = std::max({Red, Green, Blue});''',
    '''            const int Coverage =
                (
                    static_cast<int>(Red) * 54 +
                    static_cast<int>(Green) * 183 +
                    static_cast<int>(Blue) * 19
                ) >> 8;

            Alpha[
                static_cast<std::size_t>(Y) *
                static_cast<std::size_t>(BitmapWidth) +
                static_cast<std::size_t>(X)
            ] = static_cast<unsigned char>(
                std::clamp(Coverage, 0, 255)
            );'''
)
replace_once(
    "src/Rendering/SmoothTextRenderer.cpp",
    "        ? std::max(Entry->Width - 8, 0)",
    "        ? std::max(Entry->Width - 12, 0)"
)

old_draw_start = '''void SmoothTextRenderer::Draw(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color
)
{'''
new_draw_start = '''void SmoothTextRenderer::Draw(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color,
    float Opacity,
    bool Shadow
)
{'''
replace_once("src/Rendering/SmoothTextRenderer.cpp", old_draw_start, new_draw_start)

text = read("src/Rendering/SmoothTextRenderer.cpp")
start = text.index("void SmoothTextRenderer::Draw(\n")
old_body = text[start:]
new_body = '''void SmoothTextRenderer::Draw(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color,
    float Opacity,
    bool Shadow
)
{
    CachedText* Entry =
        GetOrCreate(
            Text,
            PixelHeight,
            Weight,
            TrackingEm
        );

    if (Entry == nullptr || Entry->Texture == 0)
        return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(Program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Entry->Texture);
    glUniform1i(TextureLocation, 0);

    glBindVertexArray(VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);

    auto DrawPass = [&](
        int DrawX,
        int DrawY,
        const glm::vec3& DrawColor,
        float DrawOpacity
    )
    {
        const float Left =
            static_cast<float>(DrawX) /
            static_cast<float>(Width) *
            2.0f - 1.0f;

        const float Right =
            static_cast<float>(DrawX + Entry->Width) /
            static_cast<float>(Width) *
            2.0f - 1.0f;

        const float Top =
            1.0f -
            static_cast<float>(DrawY) /
            static_cast<float>(Height) *
            2.0f;

        const float Bottom =
            1.0f -
            static_cast<float>(DrawY + Entry->Height) /
            static_cast<float>(Height) *
            2.0f;

        const float Vertices[] = {
            Left,  Top,    0.0f, 0.0f,
            Left,  Bottom, 0.0f, 1.0f,
            Right, Bottom, 1.0f, 1.0f,

            Left,  Top,    0.0f, 0.0f,
            Right, Bottom, 1.0f, 1.0f,
            Right, Top,    1.0f, 0.0f
        };

        glUniform3f(
            ColorLocation,
            DrawColor.r,
            DrawColor.g,
            DrawColor.b
        );

        glUniform1f(
            OpacityLocation,
            std::clamp(DrawOpacity, 0.0f, 1.0f)
        );

        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            sizeof(Vertices),
            Vertices
        );

        glDrawArrays(GL_TRIANGLES, 0, 6);
    };

    if (Shadow)
    {
        DrawPass(
            X,
            Y + 2,
            {0.0f, 0.0f, 0.0f},
            Opacity * 0.58f
        );

        DrawPass(
            X + 1,
            Y + 3,
            {0.0f, 0.0f, 0.0f},
            Opacity * 0.24f
        );
    }

    DrawPass(
        X,
        Y,
        Color,
        Opacity
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
'''
write("src/Rendering/SmoothTextRenderer.cpp", text[:start] + new_body)


# Renderer helper forwards opacity/shadow. Existing callers keep defaults.
replace_once(
    "src/Rendering/Renderer.h",
    '''    void DrawMenuText(
        const std::string& Text,
        int X,
        int Y,
        int PixelHeight,
        int Weight,
        float TrackingEm,
        const glm::vec3& Color
    );''',
    '''    void DrawMenuText(
        const std::string& Text,
        int X,
        int Y,
        int PixelHeight,
        int Weight,
        float TrackingEm,
        const glm::vec3& Color,
        float Opacity = 1.0f,
        bool Shadow = false
    );'''
)
replace_once(
    "src/Rendering/MenuScreen.cpp",
    '''void Renderer::DrawMenuText(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color
)''',
    '''void Renderer::DrawMenuText(
    const std::string& Text,
    int X,
    int Y,
    int PixelHeight,
    int Weight,
    float TrackingEm,
    const glm::vec3& Color,
    float Opacity,
    bool Shadow
)'''
)
replace_once(
    "src/Rendering/MenuScreen.cpp",
    '''            TrackingEm,
            Color
        );''',
    '''            TrackingEm,
            Color,
            Opacity,
            Shadow
        );'''
)

# Main-menu typography now matches style.css sizes/weights/tracking rather than
# the oversized native approximations.
menu = read("src/Rendering/MenuScreen.cpp")
for old, new in [
    ("        13,\n        500,\n        0.20f,\n        Faint", "        10,\n        400,\n        0.20f,\n        Faint"),
    ("            13,\n            500,\n            0.18f", "            10,\n            400,\n            0.20f"),
    ("        13,\n        500,\n        0.18f,\n        Faint", "        10,\n        400,\n        0.20f,\n        Faint"),
    ("            13,\n            500,\n            0.20f", "            10,\n            400,\n            0.20f"),
    ("        17,\n        600,\n        0.28f,\n        Muted", "        12,\n        400,\n        0.35f,\n        Muted"),
    ("        17,\n        600,\n        0.0f,\n        Muted", "        15,\n        600,\n        0.0f,\n        Muted"),
    ("        17,\n        800,\n        0.14f,\n        Ink", "        12,\n        800,\n        0.17f,\n        Ink"),
    ("        24,\n        700,", "        20,\n        700,"),
    ("        13,\n        500,\n        0.13f,\n        Faint", "        10,\n        400,\n        0.16f,\n        Faint"),
    ("        12,\n        500,\n        0.16f,\n        Faint", "        10,\n        400,\n        0.20f,\n        Faint"),
    ("            12,\n            500,\n            0.14f", "            10,\n            400,\n            0.20f"),
    ("        12,\n        500,\n        0.14f,\n        Faint", "        10,\n        400,\n        0.20f,\n        Faint"),
]:
    menu = menu.replace(old, new)
write("src/Rendering/MenuScreen.cpp", menu)


# Generic legacy scale -> smooth text conversion was using 8 px per glyph
# scale even though the old 3x5 glyph height was 5 * Scale. Fix that so old
# screens no longer inflate typography by ~60%.
renderer = read("src/Rendering/Renderer.cpp")
renderer = renderer.replace("const int PixelHeight = std::max(10, Scale * 8);", "const int PixelHeight = std::max(8, Scale * 5);")
renderer = renderer.replace("Scale >= 3 ? 750 : 600;", "Scale >= 3 ? 800 : 600;")
renderer = renderer.replace("Scale >= 6 ? -0.04f :\n            Scale >= 3 ? 0.035f : 0.10f;", "Scale >= 6 ? -0.06f :\n            Scale >= 3 ? 0.10f : 0.16f;")
write("src/Rendering/Renderer.cpp", renderer)

# Exact-ish old HTML crosshair dimensions: 12px box, 1px lines, 10px arms.
replace_once(
    "src/Rendering/Renderer.cpp",
    '''    glClearColor(0.92f, 0.91f, 0.82f, 1.0f);

    glScissor(CenterX - 1, CenterY - 7, 2, 14);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(CenterX - 7, CenterY - 1, 14, 2);
    glClear(GL_COLOR_BUFFER_BIT);''',
    '''    glClearColor(0.74f, 0.72f, 0.58f, 1.0f);

    glScissor(CenterX, CenterY - 5, 1, 10);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(CenterX - 5, CenterY, 10, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0.96f, 0.94f, 0.76f, 1.0f);
    glScissor(CenterX, CenterY, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);'''
)

# Stamina HUD: match the HTML horizontal label + 118x2 bar layout.
start = read("src/Rendering/Renderer.cpp")
a = start.index("void Renderer::DrawStamina(float Stamina)\n{")
b = start.index("\n\n\nnamespace\n{", a)
new_stamina = '''void Renderer::DrawStamina(float Stamina)
{
    const glm::vec3 HtmlYellow{
        244.0f / 255.0f,
        235.0f / 255.0f,
        169.0f / 255.0f
    };

    const int LabelX = 26;
    const int LabelY = static_cast<int>(Height) - 32;

    DrawMenuText(
        "SPRINT",
        LabelX,
        LabelY,
        8,
        400,
        0.18f,
        HtmlYellow,
        0.40f,
        false
    );

    const int LabelWidth =
        MenuTextWidth(
            "SPRINT",
            8,
            400,
            0.18f
        );

    const int BarX = LabelX + LabelWidth + 10;
    const int BarY = static_cast<int>(Height) - 28;
    const int BarWidth = 118;
    const int Fill = static_cast<int>(
        static_cast<float>(BarWidth) *
        std::clamp(Stamina, 0.0f, 1.0f)
    );

    DrawRect(
        BarX,
        BarY,
        BarWidth,
        2,
        {0.15f, 0.145f, 0.105f}
    );

    if (Fill > 0)
    {
        DrawRect(
            BarX,
            BarY,
            Fill,
            2,
            {0.82f, 0.79f, 0.58f}
        );
    }
}'''
write("src/Rendering/Renderer.cpp", start[:a] + new_stamina + start[b:])

# Replace gameplay HUD with the actual CSS values from the old HTML build.
text = read("src/Rendering/Renderer.cpp")
a = text.index("void Renderer::DrawHud(\n")
b = text.index("\n\nvoid Renderer::DrawStartScreen", a)
new_hud = '''void Renderer::DrawHud(
    int BreakersActive,
    int BreakersRequired,
    int InteractionType,
    bool CanExit,
    float Fps,
    const std::string& Message
)
{
    const glm::vec3 HtmlYellow{
        248.0f / 255.0f,
        239.0f / 255.0f,
        173.0f / 255.0f
    };

    DrawMenuText(
        "LEVEL 0",
        26,
        24,
        9,
        400,
        0.24f,
        HtmlYellow,
        0.40f,
        false
    );

    std::ostringstream Objective;
    Objective
        << "RESTORE POWER "
        << BreakersActive
        << "/"
        << BreakersRequired;

    DrawMenuText(
        Objective.str(),
        26,
        37,
        12,
        700,
        0.13f,
        HtmlYellow,
        0.88f,
        true
    );

    std::ostringstream FpsText;
    FpsText
        << static_cast<int>(std::round(Fps))
        << " FPS";

    const int FpsWidth =
        MenuTextWidth(
            FpsText.str(),
            10,
            400,
            0.14f
        );

    DrawMenuText(
        FpsText.str(),
        static_cast<int>(Width) - FpsWidth - 26,
        24,
        10,
        400,
        0.14f,
        HtmlYellow,
        0.44f,
        false
    );

    const std::string Version =
        std::string("V") +
        BuildVersion::Text;

    const int VersionWidth =
        MenuTextWidth(
            Version,
            9,
            400,
            0.16f
        );

    DrawMenuText(
        Version,
        static_cast<int>(Width) - VersionWidth - 26,
        40,
        9,
        400,
        0.16f,
        HtmlYellow,
        0.34f,
        false
    );

    std::string Prompt;

    if (InteractionType == 1)
        Prompt = "E  ACTIVATE BREAKER";

    if (InteractionType == 2)
        Prompt = CanExit ? "E  OPEN EXIT" : "EXIT HAS NO POWER";

    if (!Message.empty())
    {
        const int MessageHeight =
            std::clamp(
                static_cast<int>(
                    std::round(
                        static_cast<float>(Width) * 0.02f
                    )
                ),
                14,
                22
            );

        const int MessageWidth =
            MenuTextWidth(
                Message,
                MessageHeight,
                800,
                0.24f
            );

        DrawMenuText(
            Message,
            static_cast<int>(Width) / 2 -
                MessageWidth / 2,
            static_cast<int>(
                static_cast<float>(Height) * 0.19f
            ),
            MessageHeight,
            800,
            0.24f,
            {239.0f / 255.0f, 231.0f / 255.0f, 166.0f / 255.0f},
            1.0f,
            true
        );
    }

    if (!Prompt.empty())
    {
        const int PromptWidth =
            MenuTextWidth(
                Prompt,
                10,
                700,
                0.16f
            );

        const int BoxWidth = PromptWidth + 22;

        DrawRect(
            static_cast<int>(Width) / 2 - BoxWidth / 2,
            static_cast<int>(Height) - 78,
            BoxWidth,
            26,
            {0.055f, 0.05f, 0.028f}
        );

        DrawMenuText(
            Prompt,
            static_cast<int>(Width) / 2 - PromptWidth / 2,
            static_cast<int>(Height) - 70,
            10,
            700,
            0.16f,
            {255.0f / 255.0f, 248.0f / 255.0f, 192.0f / 255.0f},
            0.92f,
            true
        );
    }
}'''
write("src/Rendering/Renderer.cpp", text[:a] + new_hud + text[b:])

# End screen also uses the original CSS typography instead of the generic scale
# mapper.
text = read("src/Rendering/Renderer.cpp")
a = text.index("void Renderer::DrawEndScreen(bool Escaped)\n{")
b = text.index("\n\nvoid Renderer::EndFrame", a)
new_end = '''void Renderer::DrawEndScreen(bool Escaped)
{
    DrawRect(
        0,
        0,
        static_cast<int>(Width),
        static_cast<int>(Height),
        {0.032f, 0.03f, 0.018f}
    );

    const glm::vec3 HtmlYellow{
        235.0f / 255.0f,
        226.0f / 255.0f,
        162.0f / 255.0f
    };

    const std::string Eyebrow =
        Escaped ? "LEVEL 0 COMPLETE" : "LEVEL 0";

    const std::string Main =
        Escaped ? "YOU ESCAPED" : "YOU WERE FOUND";

    DrawMenuText(
        Eyebrow,
        48,
        52,
        10,
        400,
        0.28f,
        HtmlYellow,
        0.45f,
        false
    );

    const int MainHeight =
        std::clamp(
            static_cast<int>(
                static_cast<float>(Width) * 0.09f
            ),
            58,
            120
        );

    const int MainWidth =
        MenuTextWidth(
            Main,
            MainHeight,
            900,
            -0.075f
        );

    DrawMenuText(
        Main,
        static_cast<int>(Width) / 2 - MainWidth / 2,
        static_cast<int>(Height) / 2 - MainHeight / 2,
        MainHeight,
        900,
        -0.075f,
        {226.0f / 255.0f, 217.0f / 255.0f, 147.0f / 255.0f},
        1.0f,
        false
    );

    const std::string Restart = "R  NEW SESSION";
    const int RestartWidth =
        MenuTextWidth(
            Restart,
            12,
            800,
            0.17f
        );

    DrawMenuText(
        Restart,
        static_cast<int>(Width) / 2 - RestartWidth / 2,
        static_cast<int>(Height) / 2 + 54,
        12,
        800,
        0.17f,
        HtmlYellow,
        0.62f,
        false
    );
}'''
write("src/Rendering/Renderer.cpp", text[:a] + new_end + text[b:])

# Use the real V2 HTML-derived menu renderer; the old fallback screens were
# accidentally still wired into EndFrame.
replace_once(
    "src/Rendering/Renderer.cpp",
    '''    if (MainMenuOpen)
    {
        DrawStartScreen(Started);
        return;
    }

    if (Paused)
    {
        DrawPauseScreen();
        return;
    }

    if (!Started)
    {
        DrawStartScreen(false);
        return;
    }''',
    '''    if (MainMenuOpen)
    {
        DrawMainMenuV2(Started);
        return;
    }

    if (Paused)
    {
        DrawPauseMenuV2();
        return;
    }

    if (!Started)
    {
        DrawMainMenuV2(false);
        return;
    }'''
)

# Release notes + build notes.
write(
    "update/release_notes.txt",
    "V0.3.11 rebuilds the native text system around the original web game's actual CSS typography. The old page did not bundle a custom webfont: it used the stack Arial Narrow, Helvetica Neue, Arial, sans-serif. Windows now resolves that stack in order, uses ClearType-quality rasterization, supports CSS-like opacity and text shadows, restores the original HUD font sizes/weights/letter spacing, restores the thin 1-pixel crosshair and horizontal stamina layout, fixes the generic smooth-text scale conversion, and switches the game to the HTML-derived V2 main/pause menus instead of the leftover native fallback screens.\n"
)

readme = read("README_BUILD.txt")
readme = readme.replace("BACKROOMS OFFICAL V0.3.10", "BACKROOMS OFFICAL V0.3.11", 1)
readme = readme.replace("VERSION\n0.3.10", "VERSION\n0.3.11", 1)
readme += '''\nV0.3.11 HTML TYPOGRAPHY RESTORATION\n- Rebuilt native text styling from the original style.css instead of approximating every text role from a single scale number.\n- Original CSS font stack is now resolved in order on Windows: Arial Narrow, Helvetica Neue, Arial, sans-serif fallback.\n- Added ClearType-quality font rasterization, CSS-like opacity and HUD text shadows.\n- Gameplay HUD now uses the original web sizes, font weights, tracking, placement and brightness.\n- Restored the original thin 1-pixel crosshair and horizontal sprint label/bar layout.\n- Main/pause menus now use the HTML-derived V2 renderer instead of the leftover fallback menu.\n- Fixed generic smooth-text sizing from Scale*8 to the old 3x5 glyph-equivalent Scale*5.\n'''
write("README_BUILD.txt", readme)

print("V0.3.11 HTML typography restoration applied.")
