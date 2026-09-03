#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <sstream>

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
layout(location = 8) in vec4 iSurfaceData;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec3 vColor;
out vec3 vEmissive;
out float vRoughness;
flat out int vMaterialType;

void main()
{
    mat4 Model = mat4(iModel0, iModel1, iModel2, iModel3);
    vec4 World = Model * vec4(aPosition, 1.0);

    vWorldPosition = World.xyz;
    vNormal = normalize(mat3(transpose(inverse(Model))) * aNormal);
    vColor = iColor.rgb;
    vEmissive = iEmissiveRoughness.rgb;
    vRoughness = iEmissiveRoughness.a;
    vMaterialType = int(iSurfaceData.x + 0.5);

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
flat in int vMaterialType;

uniform vec3 uCameraPosition;
uniform int uLightCount;
uniform vec4 uLightPosition[8];
uniform vec4 uLightColor[8];

out vec4 FragColor;

float Hash21(vec2 P)
{
    P = fract(P * vec2(123.34, 456.21));
    P += dot(P, P + 45.32);
    return fract(P.x * P.y);
}

float CellEdge(float Value, float Width)
{
    float F = fract(Value);
    float D = min(F, 1.0 - F);
    return 1.0 - smoothstep(Width, Width * 2.25, D);
}

vec3 Level0Surface(vec3 BaseColor, vec3 Position, vec3 Normal, int MaterialType)
{
    vec3 Result = BaseColor;

    if (MaterialType == 1)
    {
        float Axis = abs(Normal.x) > 0.55 ? Position.z : Position.x;

        // Match the original 192 px wallpaper:
        // 24 px vertical repeat, 3.1 texture repeats across a 6 m wall.
        float StripeCell = fract(Axis / 0.24194);
        float DarkStripe =
            1.0 - smoothstep(0.06, 0.12, StripeCell);

        float BrightStripe =
            smoothstep(0.43, 0.48, StripeCell) *
            (1.0 - smoothstep(0.67, 0.72, StripeCell));

        float VerticalCell = fract(Position.y / 0.39259);
        float MotifCenter =
            1.0 - smoothstep(
                0.02,
                0.18,
                abs(VerticalCell - 0.25)
            );

        float Fiber =
            Hash21(
                floor(
                    vec2(
                        Axis * 23.0,
                        Position.y * 31.0
                    )
                )
            );

        Result *= mix(1.0, 0.955, DarkStripe);
        Result *= mix(1.0, 1.075, BrightStripe);
        Result *= mix(1.0, 0.975, MotifCenter * 0.5);
        Result *= 0.97 + (Fiber - 0.5) * 0.07;
    }
    else if (MaterialType == 2)
    {
        vec2 Carpet = Position.xz;
        float Fiber = Hash21(floor(Carpet * 24.0));
        float Fine = 0.5 + 0.5 * sin((Carpet.x + Carpet.y) * 72.0);
        float Bands = 0.5 + 0.5 * sin(Carpet.y * 21.0);

        Result *= 0.84 + Fiber * 0.20;
        Result *= 0.965 + Fine * 0.055;
        Result *= 0.975 + Bands * 0.035;
    }
    else if (MaterialType == 3)
    {
        vec2 Tile = Position.xz / 1.22;
        float GridX = CellEdge(Tile.x, 0.018);
        float GridY = CellEdge(Tile.y, 0.018);
        float Grid = max(GridX, GridY);
        float Speckle = Hash21(floor(Position.xz * 16.0));

        Result *= 0.96 + Speckle * 0.07;
        Result *= mix(1.0, 0.74, Grid * 0.48);
    }
    else if (MaterialType == 4)
    {
        float Grain = Hash21(floor(Position.xz * 36.0 + Position.yy * 7.0));
        Result *= 0.94 + Grain * 0.08;
    }
    else if (MaterialType == 5)
    {
        float Wear = Hash21(floor(Position.xz * 28.0));
        Result *= 0.94 + Wear * 0.08;
    }

    return max(Result, vec3(0.0));
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPosition - vWorldPosition);
    vec3 SurfaceColor = Level0Surface(vColor, vWorldPosition, N, vMaterialType);

    float UpFacing = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 AmbientLight =
        vec3(1.0, 0.8879, 0.6240) * 0.46;

    vec3 HemisphereLight = mix(
        vec3(0.1221, 0.1144, 0.0802),
        vec3(1.0, 0.9216, 0.7157),
        UpFacing
    ) * 0.34;

    vec3 Lighting =
        SurfaceColor *
        (AmbientLight + HemisphereLight);

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
        float SpecPower = mix(46.0, 5.0, clamp(vRoughness, 0.0, 1.0));
        float Specular = pow(max(dot(N, H), 0.0), SpecPower);
        Specular *= (1.0 - clamp(vRoughness, 0.0, 1.0)) * 0.28;

        vec3 LightColor = uLightColor[I].rgb * uLightColor[I].w;

        Lighting +=
            SurfaceColor * LightColor * Diffuse * Attenuation +
            LightColor * Specular * Attenuation;
    }

    if (vMaterialType == 6)
    {
        // The original fluorescent panel used an unlit material.
        Lighting = max(Lighting, SurfaceColor);
    }

    Lighting += vEmissive;

    float DistanceToCamera = length(vWorldPosition - uCameraPosition);
    float FogAmount = smoothstep(26.0, 72.0, DistanceToCamera);
    vec3 FogColor = vec3(0.4735, 0.4342, 0.2582);

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

    glEnableVertexAttribArray(8);
    glVertexAttribPointer(
        8,
        4,
        GL_FLOAT,
        GL_FALSE,
        Stride,
        reinterpret_cast<void*>(
            sizeof(glm::mat4) + sizeof(glm::vec4) * 2
        )
    );
    glVertexAttribDivisor(8, 1);

    glBindVertexArray(0);

    return true;
}

bool Renderer::Initialize()
{
    if (!CreateShader())
        return false;

    if (!CreateCube())
        return false;

    GhostEntityModel.Load(
        "assets/models/entity-ghost.glb",
        2.18f
    );

    DemonEntityModel.Load(
        "assets/models/entity-demon.glb",
        2.35f
    );

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
    GhostEntityModel.Shutdown();
    DemonEntityModel.Shutdown();

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

    glClearColor(0.718f, 0.690f, 0.545f, 1.0f);
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
        std::min<std::size_t>(5, Candidates.size())
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

    ActiveLightCount = Count;
    ActiveLightPositions = Positions;
    ActiveLightColors = Colors;

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
    Instance.SurfaceData = glm::vec4(
        static_cast<float>(Box.MaterialType),
        0.0f,
        0.0f,
        0.0f
    );

    Instances.push_back(Instance);
}

void Renderer::DrawBoxes(const std::vector<SceneBox>& Boxes)
{
    for (const SceneBox& Box : Boxes)
        DrawBox(Box);
}

bool Renderer::HasEntityModels() const
{
    return
        GhostEntityModel.IsReady() &&
        DemonEntityModel.IsReady();
}

void Renderer::DrawEntity(
    const glm::vec3& Position,
    const glm::vec3& Forward,
    bool DemonForm
)
{
    const EntityModel* Model =
        DemonForm
            ? &DemonEntityModel
            : &GhostEntityModel;

    if (!Model->IsReady())
    {
        Model =
            DemonForm
                ? &GhostEntityModel
                : &DemonEntityModel;
    }

    if (!Model->IsReady())
        return;

    Model->Draw(
        View,
        Projection,
        CameraPosition,
        Position,
        Forward,
        ActiveLightPositions,
        ActiveLightColors,
        ActiveLightCount
    );
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
    const int BarWidth = 118;
    const int BarHeight = 4;
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


namespace
{
    std::array<unsigned char, 5> GlyphRows(char C)
    {
        switch (C)
        {
        case 'A': return {0b010,0b101,0b111,0b101,0b101};
        case 'B': return {0b110,0b101,0b110,0b101,0b110};
        case 'C': return {0b011,0b100,0b100,0b100,0b011};
        case 'D': return {0b110,0b101,0b101,0b101,0b110};
        case 'E': return {0b111,0b100,0b110,0b100,0b111};
        case 'F': return {0b111,0b100,0b110,0b100,0b100};
        case 'G': return {0b011,0b100,0b101,0b101,0b011};
        case 'H': return {0b101,0b101,0b111,0b101,0b101};
        case 'I': return {0b111,0b010,0b010,0b010,0b111};
        case 'J': return {0b001,0b001,0b001,0b101,0b010};
        case 'K': return {0b101,0b101,0b110,0b101,0b101};
        case 'L': return {0b100,0b100,0b100,0b100,0b111};
        case 'M': return {0b101,0b111,0b111,0b101,0b101};
        case 'N': return {0b101,0b111,0b111,0b111,0b101};
        case 'O': return {0b010,0b101,0b101,0b101,0b010};
        case 'P': return {0b110,0b101,0b110,0b100,0b100};
        case 'Q': return {0b010,0b101,0b101,0b111,0b011};
        case 'R': return {0b110,0b101,0b110,0b101,0b101};
        case 'S': return {0b011,0b100,0b010,0b001,0b110};
        case 'T': return {0b111,0b010,0b010,0b010,0b010};
        case 'U': return {0b101,0b101,0b101,0b101,0b111};
        case 'V': return {0b101,0b101,0b101,0b101,0b010};
        case 'W': return {0b101,0b101,0b111,0b111,0b101};
        case 'X': return {0b101,0b101,0b010,0b101,0b101};
        case 'Y': return {0b101,0b101,0b010,0b010,0b010};
        case 'Z': return {0b111,0b001,0b010,0b100,0b111};
        case '0': return {0b111,0b101,0b101,0b101,0b111};
        case '1': return {0b010,0b110,0b010,0b010,0b111};
        case '2': return {0b110,0b001,0b010,0b100,0b111};
        case '3': return {0b110,0b001,0b010,0b001,0b110};
        case '4': return {0b101,0b101,0b111,0b001,0b001};
        case '5': return {0b111,0b100,0b110,0b001,0b110};
        case '6': return {0b011,0b100,0b111,0b101,0b111};
        case '7': return {0b111,0b001,0b010,0b010,0b010};
        case '8': return {0b111,0b101,0b111,0b101,0b111};
        case '9': return {0b111,0b101,0b111,0b001,0b110};
        case '.': return {0,0,0,0,0b010};
        case '/': return {0b001,0b001,0b010,0b100,0b100};
        case '-': return {0,0,0b111,0,0};
        case ':': return {0,0b010,0,0b010,0};
        default:  return {0,0,0,0,0};
        }
    }
}

void Renderer::DrawRect(
    int X,
    int Y,
    int RectWidth,
    int RectHeight,
    const glm::vec3& Color
)
{
    if (RectWidth <= 0 || RectHeight <= 0)
        return;

    const int Left = std::max(X, 0);
    const int Top = std::max(Y, 0);
    const int Right = std::min(
        X + RectWidth,
        static_cast<int>(Width)
    );
    const int Bottom = std::min(
        Y + RectHeight,
        static_cast<int>(Height)
    );

    if (Right <= Left || Bottom <= Top)
        return;

    glEnable(GL_SCISSOR_TEST);
    glScissor(
        Left,
        static_cast<int>(Height) - Bottom,
        Right - Left,
        Bottom - Top
    );
    glClearColor(Color.r, Color.g, Color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

int Renderer::TextWidth(const std::string& Text, int Scale) const
{
    if (Text.empty())
        return 0;

    return static_cast<int>(Text.size()) * 4 * Scale - Scale;
}

void Renderer::DrawText(
    const std::string& Text,
    int X,
    int Y,
    int Scale,
    const glm::vec3& Color
)
{
    if (Scale <= 0)
        return;

    glEnable(GL_SCISSOR_TEST);
    glClearColor(Color.r, Color.g, Color.b, 1.0f);

    int CursorX = X;

    for (char C : Text)
    {
        const auto Rows = GlyphRows(C);

        for (int Row = 0; Row < 5; ++Row)
        {
            for (int Column = 0; Column < 3; ++Column)
            {
                const unsigned char Mask =
                    static_cast<unsigned char>(1u << (2 - Column));

                if ((Rows[static_cast<std::size_t>(Row)] & Mask) == 0)
                    continue;

                const int PixelX = CursorX + Column * Scale;
                const int PixelY = Y + Row * Scale;

                if (
                    PixelX < 0 ||
                    PixelY < 0 ||
                    PixelX + Scale > static_cast<int>(Width) ||
                    PixelY + Scale > static_cast<int>(Height)
                )
                {
                    continue;
                }

                glScissor(
                    PixelX,
                    static_cast<int>(Height) - PixelY - Scale,
                    Scale,
                    Scale
                );
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }

        CursorX += 4 * Scale;
    }

    glDisable(GL_SCISSOR_TEST);
}

void Renderer::DrawHud(
    int BreakersActive,
    int BreakersRequired,
    int InteractionType,
    bool CanExit,
    float Fps
)
{
    const glm::vec3 Primary{0.92f, 0.89f, 0.65f};
    const glm::vec3 Muted{0.56f, 0.54f, 0.39f};

    DrawText("LEVEL 0", 26, 24, 2, Muted);

    std::ostringstream Objective;
    Objective
        << "RESTORE POWER  "
        << BreakersActive
        << "/"
        << BreakersRequired;

    DrawText(Objective.str(), 26, 42, 2, Primary);

    std::ostringstream FpsText;
    FpsText << static_cast<int>(std::round(Fps)) << " FPS";

    const int FpsWidth = TextWidth(FpsText.str(), 2);
    DrawText(
        FpsText.str(),
        static_cast<int>(Width) - FpsWidth - 26,
        24,
        2,
        Muted
    );

    const std::string Version = "V0.3.5";
    const int VersionWidth = TextWidth(Version, 2);
    DrawText(
        Version,
        static_cast<int>(Width) - VersionWidth - 26,
        42,
        2,
        Muted
    );

    DrawText(
        "SPRINT",
        24,
        static_cast<int>(Height) - 46,
        2,
        Muted
    );

    std::string Prompt;

    if (InteractionType == 1)
        Prompt = "E  ACTIVATE BREAKER";

    if (InteractionType == 2)
        Prompt = CanExit ? "E  OPEN EXIT" : "EXIT HAS NO POWER";

    if (!Prompt.empty())
    {
        const int PromptWidth = TextWidth(Prompt, 2);
        const int BoxWidth = PromptWidth + 22;

        DrawRect(
            static_cast<int>(Width) / 2 - BoxWidth / 2,
            static_cast<int>(Height) - 88,
            BoxWidth,
            28,
            {0.055f, 0.05f, 0.028f}
        );

        DrawText(
            Prompt,
            static_cast<int>(Width) / 2 - PromptWidth / 2,
            static_cast<int>(Height) - 81,
            2,
            Primary
        );
    }
}

void Renderer::DrawStartScreen()
{
    DrawRect(
        0,
        0,
        static_cast<int>(Width),
        static_cast<int>(Height),
        {0.035f, 0.033f, 0.020f}
    );

    const glm::vec3 Primary{0.90f, 0.85f, 0.55f};
    const glm::vec3 Muted{0.49f, 0.46f, 0.30f};
    const glm::vec3 Panel{0.085f, 0.080f, 0.048f};

    DrawText("BACKROOMS OFFICAL", 46, 42, 4, Primary);
    DrawText("V0.3.5", 48, 72, 2, Muted);

    const int CardWidth =
        std::min(700, static_cast<int>(Width) - 92);

    const int CardHeight = 260;
    const int CardX = 46;
    const int CardY =
        std::max(
            112,
            static_cast<int>(Height) / 2 - 130
        );

    DrawRect(
        CardX,
        CardY,
        CardWidth,
        CardHeight,
        Panel
    );

    DrawText("LEVEL 0", CardX + 28, CardY + 28, 3, Muted);
    DrawText("THE LOBBY", CardX + 28, CardY + 56, 5, Primary);

    DrawText(
        "RESTORE THREE BREAKERS",
        CardX + 28,
        CardY + 108,
        2,
        Muted
    );

    DrawText(
        "FIND THE POWERED EXIT",
        CardX + 28,
        CardY + 128,
        2,
        Muted
    );

    const std::string Start = "CLICK OR ENTER TO START";
    const int StartWidth = TextWidth(Start, 3);

    DrawRect(
        CardX + 28,
        CardY + 170,
        StartWidth + 26,
        34,
        {0.19f, 0.17f, 0.085f}
    );

    DrawText(
        Start,
        CardX + 41,
        CardY + 179,
        3,
        Primary
    );

    DrawText(
        "WASD  SHIFT  MOUSE  E",
        CardX + 28,
        CardY + 224,
        2,
        Muted
    );

    const std::string Cursor = "ESC RELEASES CURSOR";
    DrawText(
        Cursor,
        48,
        static_cast<int>(Height) - 42,
        2,
        Muted
    );
}

void Renderer::DrawEndScreen(bool Escaped)
{
    DrawRect(
        0,
        0,
        static_cast<int>(Width),
        static_cast<int>(Height),
        {0.032f, 0.03f, 0.018f}
    );

    const glm::vec3 Primary{0.88f, 0.84f, 0.56f};
    const glm::vec3 Muted{0.48f, 0.46f, 0.32f};

    const std::string Eyebrow =
        Escaped ? "LEVEL 0 COMPLETE" : "LEVEL 0";

    const std::string Main =
        Escaped ? "YOU ESCAPED" : "YOU WERE FOUND";

    const int MainScale = 6;
    const int MainWidth = TextWidth(Main, MainScale);

    DrawText(Eyebrow, 48, 52, 3, Muted);
    DrawText(
        Main,
        static_cast<int>(Width) / 2 - MainWidth / 2,
        static_cast<int>(Height) / 2 - 46,
        MainScale,
        Primary
    );

    const std::string Restart = "R  NEW SESSION";
    const int RestartWidth = TextWidth(Restart, 3);

    DrawText(
        Restart,
        static_cast<int>(Width) / 2 - RestartWidth / 2,
        static_cast<int>(Height) / 2 + 28,
        3,
        Muted
    );
}

void Renderer::EndFrame(
    float Stamina,
    int BreakersActive,
    int BreakersRequired,
    int InteractionType,
    bool CanExit,
    float Fps,
    bool Started,
    bool Ended,
    bool Escaped
)
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

    if (!Started)
    {
        DrawStartScreen();
        return;
    }

    if (Ended)
    {
        DrawEndScreen(Escaped);
        return;
    }

    DrawCrosshair();
    DrawStamina(Stamina);
    DrawHud(
        BreakersActive,
        BreakersRequired,
        InteractionType,
        CanExit,
        Fps
    );
}
