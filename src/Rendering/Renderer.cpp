#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace
{
    const char* VertexShaderSource = R"GLSL(
#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

layout(location = 2) in vec4 iModel0;
layout(location = 3) in vec4 iModel1;
layout(location = 4) in vec4 iModel2;
layout(location = 5) in vec4 iModel3;
layout(location = 6) in vec4 iColor;
layout(location = 7) in vec4 iEmissiveRoughness;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec3 vColor;
out vec3 vEmissive;
out float vRoughness;

void main()
{
    mat4 Model = mat4(iModel0, iModel1, iModel2, iModel3);
    vec4 World = Model * vec4(aPosition, 1.0);

    vWorldPosition = World.xyz;
    vNormal = normalize(mat3(transpose(inverse(Model))) * aNormal);
    vColor = iColor.rgb;
    vEmissive = iEmissiveRoughness.rgb;
    vRoughness = iEmissiveRoughness.a;

    gl_Position = uProjection * uView * World;
}
)GLSL";

    const char* FragmentShaderSource = R"GLSL(
#version 410 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec3 vColor;
in vec3 vEmissive;
in float vRoughness;

uniform vec3 uCameraPosition;
uniform int uLightCount;
uniform vec4 uLightPosition[8];
uniform vec4 uLightColor[8];

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPosition - vWorldPosition);

    vec3 Ambient = vColor * vec3(0.26, 0.245, 0.19);
    vec3 Lighting = Ambient;

    for (int I = 0; I < uLightCount; ++I)
    {
        vec3 ToLight = uLightPosition[I].xyz - vWorldPosition;
        float Distance = length(ToLight);
        vec3 L = ToLight / max(Distance, 0.001);

        float Radius = uLightPosition[I].w;
        float Attenuation = clamp(1.0 - Distance / Radius, 0.0, 1.0);
        Attenuation *= Attenuation;

        float Diffuse = max(dot(N, L), 0.0);

        vec3 H = normalize(L + V);
        float SpecPower = mix(48.0, 6.0, clamp(vRoughness, 0.0, 1.0));
        float Specular = pow(max(dot(N, H), 0.0), SpecPower);
        Specular *= (1.0 - clamp(vRoughness, 0.0, 1.0)) * 0.35;

        vec3 LightColor = uLightColor[I].rgb * uLightColor[I].w;

        Lighting +=
            vColor * LightColor * Diffuse * Attenuation +
            LightColor * Specular * Attenuation;
    }

    Lighting += vEmissive;

    float DistanceToCamera = length(vWorldPosition - uCameraPosition);
    float FogAmount = smoothstep(28.0, 74.0, DistanceToCamera);
    vec3 FogColor = vec3(0.56, 0.54, 0.43);

    vec3 FinalColor = mix(Lighting, FogColor, FogAmount);
    FinalColor = pow(max(FinalColor, vec3(0.0)), vec3(1.0 / 2.2));

    FragColor = vec4(FinalColor, 1.0);
}
)GLSL";
}

GLuint Renderer::CompileShader(GLenum Type, const char* Source)
{
    const GLuint Shader = glCreateShader(Type);
    glShaderSource(Shader, 1, &Source, nullptr);
    glCompileShader(Shader);

    GLint Success = GL_FALSE;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);

    if (Success == GL_FALSE)
    {
        GLint Length = 0;
        glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &Length);

        std::string Log(static_cast<std::size_t>(std::max(Length, 1)), '\0');
        glGetShaderInfoLog(Shader, Length, nullptr, Log.data());

        std::cerr << "Shader compilation failed:\n" << Log << '\n';
        glDeleteShader(Shader);
        return 0;
    }

    return Shader;
}

bool Renderer::CreateShader()
{
    const GLuint Vertex = CompileShader(GL_VERTEX_SHADER, VertexShaderSource);
    const GLuint Fragment = CompileShader(GL_FRAGMENT_SHADER, FragmentShaderSource);

    if (Vertex == 0 || Fragment == 0)
    {
        if (Vertex != 0) glDeleteShader(Vertex);
        if (Fragment != 0) glDeleteShader(Fragment);
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
        GLint Length = 0;
        glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &Length);

        std::string Log(static_cast<std::size_t>(std::max(Length, 1)), '\0');
        glGetProgramInfoLog(Program, Length, nullptr, Log.data());

        std::cerr << "Shader linking failed:\n" << Log << '\n';
        return false;
    }

    ViewLocation = glGetUniformLocation(Program, "uView");
    ProjectionLocation = glGetUniformLocation(Program, "uProjection");
    CameraLocation = glGetUniformLocation(Program, "uCameraPosition");
    LightCountLocation = glGetUniformLocation(Program, "uLightCount");
    LightPositionLocation = glGetUniformLocation(Program, "uLightPosition");
    LightColorLocation = glGetUniformLocation(Program, "uLightColor");

    return true;
}

bool Renderer::CreateCube()
{
    const float Vertices[] = {
        -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,
        -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f, 0.5f, 0.5f, 0,0,1,

         0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,
         0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,

        -0.5f,-0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f, 0.5f,-1,0,0,
        -0.5f,-0.5f,-0.5f,-1,0,0,  -0.5f, 0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f,-0.5f,-1,0,0,

         0.5f,-0.5f, 0.5f, 1,0,0,   0.5f,-0.5f,-0.5f, 1,0,0,   0.5f, 0.5f,-0.5f, 1,0,0,
         0.5f,-0.5f, 0.5f, 1,0,0,   0.5f, 0.5f,-0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,

        -0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,
        -0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,  -0.5f, 0.5f,-0.5f, 0,1,0,

        -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0,
        -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f, 0,-1,0
    };

    glGenVertexArrays(1, &VertexArray);
    glBindVertexArray(VertexArray);

    glGenBuffers(1, &VertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glGenBuffers(1, &InstanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

    const GLsizei Stride = sizeof(InstanceData);

    for (int Column = 0; Column < 4; ++Column)
    {
        const GLuint Location = static_cast<GLuint>(2 + Column);
        glEnableVertexAttribArray(Location);
        glVertexAttribPointer(
            Location,
            4,
            GL_FLOAT,
            GL_FALSE,
            Stride,
            reinterpret_cast<void*>(
                static_cast<std::size_t>(Column) * sizeof(glm::vec4)
            )
        );
        glVertexAttribDivisor(Location, 1);
    }

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(
        6,
        4,
        GL_FLOAT,
        GL_FALSE,
        Stride,
        reinterpret_cast<void*>(sizeof(glm::mat4))
    );
    glVertexAttribDivisor(6, 1);

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(
        7,
        4,
        GL_FLOAT,
        GL_FALSE,
        Stride,
        reinterpret_cast<void*>(sizeof(glm::mat4) + sizeof(glm::vec4))
    );
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);

    return true;
}

bool Renderer::Initialize()
{
    if (!CreateShader())
        return false;

    if (!CreateCube())
        return false;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glDisable(GL_BLEND);

    Instances.reserve(2048);

    return true;
}

void Renderer::Shutdown()
{
    if (InstanceBuffer != 0)
        glDeleteBuffers(1, &InstanceBuffer);

    if (VertexBuffer != 0)
        glDeleteBuffers(1, &VertexBuffer);

    if (VertexArray != 0)
        glDeleteVertexArrays(1, &VertexArray);

    if (Program != 0)
        glDeleteProgram(Program);

    InstanceBuffer = 0;
    VertexBuffer = 0;
    VertexArray = 0;
    Program = 0;
}

void Renderer::Resize(uint32_t NewWidth, uint32_t NewHeight)
{
    Width = std::max(NewWidth, 1u);
    Height = std::max(NewHeight, 1u);

    glViewport(
        0,
        0,
        static_cast<GLsizei>(Width),
        static_cast<GLsizei>(Height)
    );
}

void Renderer::BeginFrame()
{
    Instances.clear();

    glClearColor(0.56f, 0.54f, 0.43f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetCamera(
    const glm::mat4& NewView,
    const glm::mat4& NewProjection,
    const glm::vec3& Position
)
{
    View = NewView;
    Projection = NewProjection;
    CameraPosition = Position;
}

void Renderer::SetLights(
    const std::vector<LightPoint>& Lights,
    const glm::vec3& ViewerPosition,
    float Time
)
{
    struct Candidate
    {
        const LightPoint* Light = nullptr;
        float DistanceSquared = 0.0f;
    };

    std::vector<Candidate> Candidates;
    Candidates.reserve(Lights.size());

    for (const LightPoint& Light : Lights)
    {
        const glm::vec3 Delta = Light.Position - ViewerPosition;
        Candidates.push_back({&Light, glm::dot(Delta, Delta)});
    }

    std::sort(
        Candidates.begin(),
        Candidates.end(),
        [](const Candidate& A, const Candidate& B)
        {
            return A.DistanceSquared < B.DistanceSquared;
        }
    );

    std::array<glm::vec4, 8> Positions{};
    std::array<glm::vec4, 8> Colors{};

    const int Count = static_cast<int>(
        std::min<std::size_t>(8, Candidates.size())
    );

    for (int I = 0; I < Count; ++I)
    {
        const LightPoint& Light = *Candidates[static_cast<std::size_t>(I)].Light;

        const float Pulse = std::sin(
            Time * Light.FlickerSpeed + Light.Phase
        );

        const float Glitch = std::sin(
            Time * Light.FlickerSpeed * 1.73f +
            Light.Phase * 2.4f
        );

        const float Drop =
            Light.Faulty && Glitch < -0.965f
                ? 0.28f
                : 1.0f;

        const float Intensity =
            Light.BaseIntensity *
            (0.96f + Pulse * Light.FlickerStrength) *
            Drop;

        Positions[static_cast<std::size_t>(I)] =
            glm::vec4(Light.Position, 13.5f);

        Colors[static_cast<std::size_t>(I)] =
            glm::vec4(Light.Color, Intensity);
    }

    glUseProgram(Program);

    glUniform1i(LightCountLocation, Count);

    if (Count > 0)
    {
        glUniform4fv(
            LightPositionLocation,
            Count,
            glm::value_ptr(Positions[0])
        );

        glUniform4fv(
            LightColorLocation,
            Count,
            glm::value_ptr(Colors[0])
        );
    }
}

void Renderer::DrawBox(const SceneBox& Box)
{
    InstanceData Instance;

    Instance.Model =
        glm::translate(glm::mat4(1.0f), Box.Position) *
        glm::scale(glm::mat4(1.0f), Box.Size);

    Instance.Color = glm::vec4(Box.Color, 1.0f);
    Instance.EmissiveRoughness =
        glm::vec4(Box.Emissive, Box.Roughness);

    Instances.push_back(Instance);
}

void Renderer::DrawBoxes(const std::vector<SceneBox>& Boxes)
{
    for (const SceneBox& Box : Boxes)
        DrawBox(Box);
}

void Renderer::DrawCrosshair()
{
    const GLint CenterX = static_cast<GLint>(Width / 2);
    const GLint CenterY = static_cast<GLint>(Height / 2);

    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.92f, 0.91f, 0.82f, 1.0f);

    glScissor(CenterX - 1, CenterY - 7, 2, 14);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(CenterX - 7, CenterY - 1, 14, 2);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);
}

void Renderer::DrawStamina(float Stamina)
{
    const int Margin = 24;
    const int BarWidth = 240;
    const int BarHeight = 8;
    const int Fill = static_cast<int>(
        static_cast<float>(BarWidth) *
        std::clamp(Stamina, 0.0f, 1.0f)
    );

    glEnable(GL_SCISSOR_TEST);

    glScissor(
        Margin,
        Margin,
        BarWidth,
        BarHeight
    );
    glClearColor(0.12f, 0.12f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (Fill > 0)
    {
        glScissor(
            Margin,
            Margin,
            Fill,
            BarHeight
        );
        glClearColor(0.82f, 0.79f, 0.58f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glDisable(GL_SCISSOR_TEST);
}

void Renderer::EndFrame(float Stamina)
{
    glUseProgram(Program);

    glUniformMatrix4fv(
        ViewLocation,
        1,
        GL_FALSE,
        glm::value_ptr(View)
    );

    glUniformMatrix4fv(
        ProjectionLocation,
        1,
        GL_FALSE,
        glm::value_ptr(Projection)
    );

    glUniform3fv(
        CameraLocation,
        1,
        glm::value_ptr(CameraPosition)
    );

    glBindVertexArray(VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);

    if (!Instances.empty())
    {
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                Instances.size() * sizeof(InstanceData)
            ),
            Instances.data(),
            GL_DYNAMIC_DRAW
        );

        glDrawArraysInstanced(
            GL_TRIANGLES,
            0,
            36,
            static_cast<GLsizei>(Instances.size())
        );
    }

    glBindVertexArray(0);

    DrawCrosshair();
    DrawStamina(Stamina);
}
