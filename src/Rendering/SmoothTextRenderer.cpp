#include "SmoothTextRenderer.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <sstream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace
{
    const char* TextVertexShader = R"GLSL(
#version 410 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUv;
out vec2 vUv;
void main()
{
    vUv = aUv;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)GLSL";

    const char* TextFragmentShader = R"GLSL(
#version 410 core
in vec2 vUv;
uniform sampler2D uTexture;
uniform vec3 uColor;
uniform float uOpacity;
out vec4 FragColor;
void main()
{
    float Alpha = texture(uTexture, vUv).r;
    FragColor = vec4(uColor, Alpha * uOpacity);
}
)GLSL";

#ifdef _WIN32
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
            L"Segoe UI Variable",
            L"Segoe UI",
            L"Arial",
            L"Tahoma"
        };

        for (const wchar_t* Face : FontStack)
        {
            if (HasFontFace(DeviceContext, Face))
                return Face;
        }

        return L"Segoe UI";
    }
#endif

    GLuint Compile(GLenum Type, const char* Source)
    {
        const GLuint Shader = glCreateShader(Type);
        glShaderSource(Shader, 1, &Source, nullptr);
        glCompileShader(Shader);

        GLint Success = GL_FALSE;
        glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);

        if (Success == GL_FALSE)
        {
            glDeleteShader(Shader);
            return 0;
        }

        return Shader;
    }
}

bool SmoothTextRenderer::Initialize()
{
#ifdef _WIN32
    Ready = CreateShader();
    return Ready;
#else
    Ready = false;
    return false;
#endif
}

void SmoothTextRenderer::Shutdown()
{
    for (auto& Pair : Cache)
    {
        if (Pair.second.Texture != 0)
            glDeleteTextures(1, &Pair.second.Texture);
    }

    Cache.clear();

    if (VertexBuffer != 0)
        glDeleteBuffers(1, &VertexBuffer);

    if (VertexArray != 0)
        glDeleteVertexArrays(1, &VertexArray);

    if (Program != 0)
        glDeleteProgram(Program);

    VertexBuffer = 0;
    VertexArray = 0;
    Program = 0;
    Ready = false;
}

void SmoothTextRenderer::Resize(uint32_t NewWidth, uint32_t NewHeight)
{
    Width = std::max(NewWidth, 1u);
    Height = std::max(NewHeight, 1u);
}

bool SmoothTextRenderer::IsReady() const
{
    return Ready;
}

bool SmoothTextRenderer::CreateShader()
{
    const GLuint Vertex = Compile(GL_VERTEX_SHADER, TextVertexShader);
    const GLuint Fragment = Compile(GL_FRAGMENT_SHADER, TextFragmentShader);

    if (Vertex == 0 || Fragment == 0)
    {
        if (Vertex != 0)
            glDeleteShader(Vertex);

        if (Fragment != 0)
            glDeleteShader(Fragment);

        return false;
    }

    Program = glCreateProgram();
    glAttachShader(Program, Vertex);
    glAttachShader(Program, Fragment);
    glLinkProgram(Program);

    glDeleteShader(Vertex);
    glDeleteShader(Fragment);

    GLint Success = GL_FALSE;
    glGetProgramiv(Program, GL_LINK_STATUS, &Success);

    if (Success == GL_FALSE)
    {
        glDeleteProgram(Program);
        Program = 0;
        return false;
    }

    ColorLocation = glGetUniformLocation(Program, "uColor");
    OpacityLocation = glGetUniformLocation(Program, "uOpacity");
    TextureLocation = glGetUniformLocation(Program, "uTexture");

    glGenVertexArrays(1, &VertexArray);
    glGenBuffers(1, &VertexBuffer);

    glBindVertexArray(VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * 6 * 4,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(float) * 4,
        reinterpret_cast<void*>(sizeof(float) * 2)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

std::string SmoothTextRenderer::CacheKey(
    const std::string& Text,
    int PixelHeight,
    int Weight,
    float TrackingEm
)
{
    std::ostringstream Stream;
    Stream
        << PixelHeight
        << '|'
        << Weight
        << '|'
        << static_cast<int>(std::round(TrackingEm * 1000.0f))
        << '|'
        << Text;

    return Stream.str();
}

SmoothTextRenderer::CachedText* SmoothTextRenderer::GetOrCreate(
    const std::string& Text,
    int PixelHeight,
    int Weight,
    float TrackingEm
)
{
#ifdef _WIN32
    if (!Ready || Text.empty() || PixelHeight <= 0)
        return nullptr;

    const std::string Key =
        CacheKey(
            Text,
            PixelHeight,
            Weight,
            TrackingEm
        );

    auto Existing = Cache.find(Key);

    if (Existing != Cache.end())
        return &Existing->second;

    std::wstring Wide;
    const int WideLength = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Text.c_str(),
        static_cast<int>(Text.size()),
        nullptr,
        0
    );

    if (WideLength > 0)
    {
        Wide.resize(static_cast<std::size_t>(WideLength));
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            Text.c_str(),
            static_cast<int>(Text.size()),
            Wide.data(),
            WideLength
        );
    }
    else
    {
        Wide.reserve(Text.size());
        for (unsigned char Character : Text)
            Wide.push_back(static_cast<wchar_t>(Character));
    }

    HDC DeviceContext = CreateCompatibleDC(nullptr);

    if (DeviceContext == nullptr)
        return nullptr;

    const int CharacterExtra =
        static_cast<int>(
            std::round(
                static_cast<float>(PixelHeight) *
                TrackingEm
            )
        );

    const wchar_t* CssFace =
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
    );

    if (Font == nullptr)
    {
        DeleteDC(DeviceContext);
        return nullptr;
    }

    HGDIOBJ OldFont = SelectObject(DeviceContext, Font);
    SetTextCharacterExtra(DeviceContext, CharacterExtra);

    SIZE Size{};

    if (
        !GetTextExtentPoint32W(
            DeviceContext,
            Wide.c_str(),
            static_cast<int>(Wide.size()),
            &Size
        )
    )
    {
        SelectObject(DeviceContext, OldFont);
        DeleteObject(Font);
        DeleteDC(DeviceContext);
        return nullptr;
    }

    TEXTMETRICW Metrics{};
    GetTextMetricsW(DeviceContext, &Metrics);

    const int BitmapWidth =
        std::max(Size.cx + 12, 1L);

    const int BitmapHeight =
        std::max(
            static_cast<int>(Metrics.tmHeight) +
                static_cast<int>(Metrics.tmExternalLeading) +
                12,
            1
        );

    BITMAPINFO Info{};
    Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    Info.bmiHeader.biWidth = BitmapWidth;
    Info.bmiHeader.biHeight = -BitmapHeight;
    Info.bmiHeader.biPlanes = 1;
    Info.bmiHeader.biBitCount = 32;
    Info.bmiHeader.biCompression = BI_RGB;

    void* Pixels = nullptr;

    HBITMAP Bitmap = CreateDIBSection(
        DeviceContext,
        &Info,
        DIB_RGB_COLORS,
        &Pixels,
        nullptr,
        0
    );

    if (Bitmap == nullptr || Pixels == nullptr)
    {
        SelectObject(DeviceContext, OldFont);
        DeleteObject(Font);
        DeleteDC(DeviceContext);
        return nullptr;
    }

    HGDIOBJ OldBitmap = SelectObject(DeviceContext, Bitmap);

    PatBlt(
        DeviceContext,
        0,
        0,
        BitmapWidth,
        BitmapHeight,
        BLACKNESS
    );

    SetBkMode(DeviceContext, TRANSPARENT);
    SetTextColor(DeviceContext, RGB(255, 255, 255));
    SetTextAlign(DeviceContext, TA_LEFT | TA_TOP);
    SetTextCharacterExtra(DeviceContext, CharacterExtra);

    TextOutW(
        DeviceContext,
        6,
        3,
        Wide.c_str(),
        static_cast<int>(Wide.size())
    );

    const auto* Source =
        static_cast<const unsigned char*>(Pixels);

    std::vector<unsigned char> Alpha(
        static_cast<std::size_t>(BitmapWidth) *
        static_cast<std::size_t>(BitmapHeight),
        0
    );

    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        for (int X = 0; X < BitmapWidth; ++X)
        {
            const std::size_t SourceIndex =
                (
                    static_cast<std::size_t>(Y) *
                    static_cast<std::size_t>(BitmapWidth) +
                    static_cast<std::size_t>(X)
                ) * 4;

            const unsigned char Blue = Source[SourceIndex + 0];
            const unsigned char Green = Source[SourceIndex + 1];
            const unsigned char Red = Source[SourceIndex + 2];

            const int Coverage =
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
            );
        }
    }

    SelectObject(DeviceContext, OldBitmap);
    SelectObject(DeviceContext, OldFont);
    DeleteObject(Bitmap);
    DeleteObject(Font);
    DeleteDC(DeviceContext);

    CachedText Entry;
    Entry.Width = BitmapWidth;
    Entry.Height = BitmapHeight;

    glGenTextures(1, &Entry.Texture);
    glBindTexture(GL_TEXTURE_2D, Entry.Texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        BitmapWidth,
        BitmapHeight,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        Alpha.data()
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    auto Inserted = Cache.emplace(Key, Entry);
    return &Inserted.first->second;
#else
    static_cast<void>(Text);
    static_cast<void>(PixelHeight);
    static_cast<void>(Weight);
    static_cast<void>(TrackingEm);
    return nullptr;
#endif
}

int SmoothTextRenderer::Measure(
    const std::string& Text,
    int PixelHeight,
    int Weight,
    float TrackingEm
)
{
    CachedText* Entry =
        GetOrCreate(
            Text,
            PixelHeight,
            Weight,
            TrackingEm
        );

    return Entry != nullptr
        ? std::max(Entry->Width - 12, 0)
        : 0;
}

void SmoothTextRenderer::Draw(
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
