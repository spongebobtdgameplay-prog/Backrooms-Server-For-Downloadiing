#pragma once

#include "../World/WorldTypes.h"
#include "EntityModel.h"
#include "GameTextRenderer.h"
#include "SmoothTextRenderer.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct UpdateVisualState;

enum class MenuUiAction
{
    None,
    Primary,
    NewSession,
    Resume,
    MainMenu
};

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

    bool HasBreakerModel() const;
    void DrawBreaker(
        const glm::vec3& Position,
        const glm::vec3& Forward
    );

    bool HasExitDoorModel() const;
    void DrawExitDoor(
        const glm::vec3& Position,
        const glm::vec3& Forward
    );

    bool HasEntityModels() const;
    void DrawEntity(
        const glm::vec3& Position,
        const glm::vec3& Forward,
        bool DemonForm,
        bool PreviousDemonForm,
        float ShiftProgress
    );

    void DrawMainMenuV2(bool HasSession);
    void DrawPauseMenuV2();

    void DrawMainMenuV3(bool HasSession);
    void DrawPauseMenuV3();
    void DrawGameplayOverlayV3(
        float Stamina,
        int BreakersActive,
        int BreakersRequired,
        int InteractionType,
        bool CanExit,
        float Fps
    );

    void SetMenuPointer(float X, float Y);
    void ClearMenuPointer();
    void UpdateInterface(float DeltaTime);
    MenuUiAction HitTestMainMenu(bool HasSession) const;
    MenuUiAction HitTestPauseMenu() const;
    void ShutdownInterfaceV3();

    void DrawUpdateScreen(
        const UpdateVisualState& State
    );

    void DrawUpdateScreenV2(
        const UpdateVisualState& State
    );

    void EndFrame(
        float Stamina,
        int BreakersActive,
        int BreakersRequired,
        int InteractionType,
        bool CanExit,
        float Fps,
        const std::string& Message,
        bool Started,
        bool MainMenuOpen,
        bool Paused,
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

    struct UiRect
    {
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;

        bool Contains(float PointX, float PointY) const
        {
            return
                PointX >= static_cast<float>(X) &&
                PointY >= static_cast<float>(Y) &&
                PointX < static_cast<float>(X + Width) &&
                PointY < static_cast<float>(Y + Height);
        }
    };

    bool CreateShader();
    bool CreateCube();
    bool CreateShadowResources();
    void DestroyShadowResources();
    void RenderShadowMap();
    GLuint CompileShader(GLenum Type, const char* Source);
    void DrawCrosshair();
    void DrawStamina(float Stamina);
    void DrawHud(
        int BreakersActive,
        int BreakersRequired,
        int InteractionType,
        bool CanExit,
        float Fps,
        const std::string& Message
    );
    void DrawStartScreen(bool HasSession);
    void DrawPauseScreen();
    void DrawEndScreen(bool Escaped);
    void DrawMenuBackdrop();
    void DrawMenuText(
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
    int MenuTextWidth(
        const std::string& Text,
        int PixelHeight,
        int Weight,
        float TrackingEm
    );
    void DrawText(
        const std::string& Text,
        int X,
        int Y,
        int Scale,
        const glm::vec3& Color
    );
    int TextWidth(const std::string& Text, int Scale);
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
    GLint ShadowMatrixLocation = -1;
    GLint ShadowMapLocation = -1;
    GLint ShadowEnabledLocation = -1;

    GLuint ShadowProgram = 0;
    GLuint ShadowFramebuffer = 0;
    GLuint ShadowDepthTexture = 0;
    GLint ShadowDepthMatrixLocation = -1;

    uint32_t Width = 1600;
    uint32_t Height = 900;

    glm::mat4 View{1.0f};
    glm::mat4 Projection{1.0f};
    glm::vec3 CameraPosition{0.0f};
    glm::mat4 ShadowLightMatrix{1.0f};

    bool ShadowEnabled = false;

    std::vector<InstanceData> Instances;

    EntityModel BreakerModel;
    EntityModel ExitDoorModel;
    EntityModel GhostEntityModel;
    EntityModel DemonEntityModel;
    SmoothTextRenderer MenuTextRenderer;
    GameTextRenderer GameplayTextRenderer;

    UiRect MainPrimaryRect;
    UiRect MainSecondaryRect;
    UiRect PauseResumeRect;
    UiRect PauseMainMenuRect;

    float MenuPointerX = -10000.0f;
    float MenuPointerY = -10000.0f;

    float MainPrimaryHover = 0.0f;
    float MainSecondaryHover = 0.0f;
    float PauseResumeHover = 0.0f;
    float PauseMainMenuHover = 0.0f;

    std::array<glm::vec4, 8> ActiveLightPositions{};
    std::array<glm::vec4, 8> ActiveLightColors{};
    int ActiveLightCount = 0;
};
