#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

class SmoothTextRenderer
{
public:
    SmoothTextRenderer() = default;
    ~SmoothTextRenderer();

    SmoothTextRenderer(const SmoothTextRenderer&) = delete;
    SmoothTextRenderer& operator=(const SmoothTextRenderer&) = delete;

    bool Initialize();
    void Shutdown();
    void Resize(uint32_t Width, uint32_t Height);

    bool IsReady() const;

    int Measure(
        const std::string& Text,
        int PixelHeight,
        int Weight,
        float TrackingEm
    );

    void Draw(
        const std::string& Text,
        int X,
        int Y,
        int PixelHeight,
        int Weight,
        float TrackingEm,
        const glm::vec3& Color,
        float Opacity = 1.0f,
        bool Shadow = false
    );

private:
    struct CachedText
    {
        GLuint Texture = 0;
        int Width = 0;
        int Height = 0;
    };

    bool CreateShader();

    CachedText* GetOrCreate(
        const std::string& Text,
        int PixelHeight,
        int Weight,
        float TrackingEm
    );

    static std::string CacheKey(
        const std::string& Text,
        int PixelHeight,
        int Weight,
        float TrackingEm
    );

    GLuint Program = 0;
    GLuint VertexArray = 0;
    GLuint VertexBuffer = 0;

    GLint ColorLocation = -1;
    GLint OpacityLocation = -1;
    GLint TextureLocation = -1;

    uint32_t Width = 1600;
    uint32_t Height = 900;

    bool Ready = false;

    std::unordered_map<std::string, CachedText> Cache;
};
