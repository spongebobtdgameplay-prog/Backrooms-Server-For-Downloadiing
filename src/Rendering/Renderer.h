#pragma once

#include "../World/WorldTypes.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class Renderer
{
public:
    bool Initialize();
    void Shutdown();

    void Resize(uint32_t Width, uint32_t Height);

    void BeginFrame();
    void SetCamera(
        const glm::mat4& View,
        const glm::mat4& Projection,
        const glm::vec3& Position
    );

    void SetLights(
        const std::vector<LightPoint>& Lights,
        const glm::vec3& ViewerPosition,
        float Time
    );

    void DrawBox(const SceneBox& Box);
    void DrawBoxes(const std::vector<SceneBox>& Boxes);
    void EndFrame(
        float Stamina,
        int BreakersActive,
        int BreakersRequired,
        int InteractionType,
        bool CanExit,
        float Fps,
        bool Ended,
        bool Escaped
    );

private:
    struct InstanceData
    {
        glm::mat4 Model{1.0f};
        glm::vec4 Color{1.0f};
        glm::vec4 EmissiveRoughness{0.0f};
        glm::vec4 SurfaceData{0.0f};
    };

    bool CreateShader();
    bool CreateCube();
    GLuint CompileShader(GLenum Type, const char* Source);
    void DrawCrosshair();
    void DrawStamina(float Stamina);
    void DrawHud(
        int BreakersActive,
        int BreakersRequired,
        int InteractionType,
        bool CanExit,
        float Fps
    );
    void DrawEndScreen(bool Escaped);
    void DrawText(
        const std::string& Text,
        int X,
        int Y,
        int Scale,
        const glm::vec3& Color
    );
    int TextWidth(const std::string& Text, int Scale) const;
    void DrawRect(
        int X,
        int Y,
        int RectWidth,
        int RectHeight,
        const glm::vec3& Color
    );

    GLuint Program = 0;
    GLuint VertexArray = 0;
    GLuint VertexBuffer = 0;
    GLuint InstanceBuffer = 0;

    GLint ViewLocation = -1;
    GLint ProjectionLocation = -1;
    GLint CameraLocation = -1;
    GLint LightCountLocation = -1;
    GLint LightPositionLocation = -1;
    GLint LightColorLocation = -1;

    uint32_t Width = 1600;
    uint32_t Height = 900;

    glm::mat4 View{1.0f};
    glm::mat4 Projection{1.0f};
    glm::vec3 CameraPosition{0.0f};

    std::vector<InstanceData> Instances;
};
