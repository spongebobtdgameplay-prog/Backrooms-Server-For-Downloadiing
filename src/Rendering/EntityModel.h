#pragma once

#include "../World/WorldTypes.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

class EntityModel
{
public:
    EntityModel() = default;
    ~EntityModel();

    EntityModel(const EntityModel&) = delete;
    EntityModel& operator=(const EntityModel&) = delete;

    bool Load(const std::string& Path, float TargetHeight);
    void Shutdown();

    bool IsReady() const;

    void Draw(
        const glm::mat4& View,
        const glm::mat4& Projection,
        const glm::vec3& CameraPosition,
        const glm::vec3& Position,
        const glm::vec3& Forward,
        const std::array<glm::vec4, 8>& LightPositions,
        const std::array<glm::vec4, 8>& LightColors,
        int LightCount
    ) const;

private:
    struct Vertex
    {
        glm::vec3 Position{0.0f};
        glm::vec3 Normal{0.0f, 1.0f, 0.0f};
        glm::vec2 TexCoord{0.0f};
    };

    struct Primitive
    {
        GLuint VertexArray = 0;
        GLuint VertexBuffer = 0;
        GLuint IndexBuffer = 0;
        GLuint Texture = 0;
        GLsizei IndexCount = 0;
        glm::vec4 BaseColor{1.0f};
    };

    bool CreateShader();
    GLuint CompileShader(GLenum Type, const char* Source) const;
    GLuint CreateTexture(
        const unsigned char* Data,
        std::size_t Size
    ) const;
    GLuint CreateWhiteTexture() const;

    GLuint Program = 0;

    GLint ModelLocation = -1;
    GLint ViewLocation = -1;
    GLint ProjectionLocation = -1;
    GLint CameraLocation = -1;
    GLint LightCountLocation = -1;
    GLint LightPositionLocation = -1;
    GLint LightColorLocation = -1;
    GLint BaseColorLocation = -1;
    GLint TextureLocation = -1;

    std::vector<Primitive> Primitives;

    glm::vec3 BoundsMin{0.0f};
    glm::vec3 BoundsMax{0.0f};
    glm::vec3 Center{0.0f};
    float GroundY = 0.0f;
    float Scale = 1.0f;

    bool Ready = false;
};
