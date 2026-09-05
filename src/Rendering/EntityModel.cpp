#include "EntityModel.h"

#define CGLTF_IMPLEMENTATION
#include "../../third_party/cgltf/cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "../../third_party/stb/stb_image.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <vector>

namespace
{
    const char* EntityVertexShader = R"GLSL(
#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec2 vTexCoord;

void main()
{
    vec4 World = uModel * vec4(aPosition, 1.0);
    vWorldPosition = World.xyz;
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * World;
}
)GLSL";

    const char* EntityFragmentShader = R"GLSL(
#version 410 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform vec4 uBaseColor;
uniform vec3 uCameraPosition;
uniform int uLightCount;
uniform vec4 uLightPosition[8];
uniform vec4 uLightColor[8];

out vec4 FragColor;

void main()
{
    vec4 Texel = texture(uTexture, vTexCoord) * uBaseColor;

    if (Texel.a < 0.08)
        discard;

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPosition - vWorldPosition);

    float UpFacing =
        clamp(N.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 AmbientLight =
        vec3(1.0, 0.8879, 0.6240) * 0.46;

    vec3 HemisphereLight = mix(
        vec3(0.1221, 0.1144, 0.0802),
        vec3(1.0, 0.9216, 0.7157),
        UpFacing
    ) * 0.34;

    vec3 Lighting =
        Texel.rgb *
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
        float Specular = pow(max(dot(N, H), 0.0), 18.0) * 0.08;

        vec3 LightColor = uLightColor[I].rgb * uLightColor[I].w;

        Lighting +=
            Texel.rgb * LightColor * Diffuse * Attenuation +
            LightColor * Specular * Attenuation;
    }

    float DistanceToCamera = length(vWorldPosition - uCameraPosition);
    float FogAmount = smoothstep(26.0, 72.0, DistanceToCamera);
    vec3 FogColor = vec3(0.4735, 0.4342, 0.2582);

    vec3 FinalColor = mix(Lighting, FogColor, FogAmount);
    FinalColor = pow(max(FinalColor, vec3(0.0)), vec3(1.0 / 2.2));

    FragColor = vec4(FinalColor, Texel.a);
}
)GLSL";

    const cgltf_accessor* FindAttribute(
        const cgltf_primitive& Primitive,
        cgltf_attribute_type Type
    )
    {
        for (cgltf_size I = 0; I < Primitive.attributes_count; ++I)
        {
            const cgltf_attribute& Attribute = Primitive.attributes[I];

            if (
                Attribute.type == Type &&
                Attribute.data != nullptr
            )
            {
                return Attribute.data;
            }
        }

        return nullptr;
    }

    glm::mat4 NodeWorldMatrix(const cgltf_node& Node)
    {
        float Matrix[16] = {};
        cgltf_node_transform_world(&Node, Matrix);
        return glm::make_mat4(Matrix);
    }
}

EntityModel::~EntityModel()
{
    Shutdown();
}

GLuint EntityModel::CompileShader(
    GLenum Type,
    const char* Source
) const
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

        std::string Log(
            static_cast<std::size_t>(std::max(Length, 1)),
            '\0'
        );

        glGetShaderInfoLog(
            Shader,
            Length,
            nullptr,
            Log.data()
        );

        std::cerr
            << "Entity shader compilation failed:\n"
            << Log
            << '\n';

        glDeleteShader(Shader);
        return 0;
    }

    return Shader;
}

bool EntityModel::CreateShader()
{
    const GLuint Vertex =
        CompileShader(GL_VERTEX_SHADER, EntityVertexShader);

    const GLuint Fragment =
        CompileShader(GL_FRAGMENT_SHADER, EntityFragmentShader);

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
        GLint Length = 0;
        glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &Length);

        std::string Log(
            static_cast<std::size_t>(std::max(Length, 1)),
            '\0'
        );

        glGetProgramInfoLog(
            Program,
            Length,
            nullptr,
            Log.data()
        );

        std::cerr
            << "Entity shader linking failed:\n"
            << Log
            << '\n';

        glDeleteProgram(Program);
        Program = 0;
        return false;
    }

    ModelLocation = glGetUniformLocation(Program, "uModel");
    ViewLocation = glGetUniformLocation(Program, "uView");
    ProjectionLocation = glGetUniformLocation(Program, "uProjection");
    CameraLocation = glGetUniformLocation(Program, "uCameraPosition");
    LightCountLocation = glGetUniformLocation(Program, "uLightCount");
    LightPositionLocation = glGetUniformLocation(Program, "uLightPosition");
    LightColorLocation = glGetUniformLocation(Program, "uLightColor");
    BaseColorLocation = glGetUniformLocation(Program, "uBaseColor");
    TextureLocation = glGetUniformLocation(Program, "uTexture");

    return true;
}

GLuint EntityModel::CreateWhiteTexture() const
{
    const unsigned char Pixel[4] = {255, 255, 255, 255};

    GLuint Texture = 0;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        1,
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        Pixel
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
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    return Texture;
}

GLuint EntityModel::CreateTexture(
    const unsigned char* Data,
    std::size_t Size
) const
{
    if (Data == nullptr || Size == 0)
        return CreateWhiteTexture();

    int Width = 0;
    int Height = 0;
    int Channels = 0;

    stbi_uc* Pixels = stbi_load_from_memory(
        Data,
        static_cast<int>(Size),
        &Width,
        &Height,
        &Channels,
        STBI_rgb_alpha
    );

    if (
        Pixels == nullptr ||
        Width <= 0 ||
        Height <= 0
    )
    {
        if (Pixels != nullptr)
            stbi_image_free(Pixels);

        return CreateWhiteTexture();
    }

    GLuint Texture = 0;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_SRGB8_ALPHA8,
        Width,
        Height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        Pixels
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    stbi_image_free(Pixels);
    return Texture;
}

bool EntityModel::Load(
    const std::string& Path,
    float TargetHeight
)
{
    Shutdown();

    if (!CreateShader())
        return false;

    const std::filesystem::path ModelDirectory =
        std::filesystem::path(Path).parent_path();

    cgltf_options Options{};
    cgltf_data* Data = nullptr;

    cgltf_result Result = cgltf_parse_file(
        &Options,
        Path.c_str(),
        &Data
    );

    if (
        Result != cgltf_result_success ||
        Data == nullptr
    )
    {
        std::cerr
            << "Could not parse entity model: "
            << Path
            << '\n';

        Shutdown();
        return false;
    }

    Result = cgltf_load_buffers(
        &Options,
        Data,
        Path.c_str()
    );

    if (Result != cgltf_result_success)
    {
        std::cerr
            << "Could not load entity model buffers: "
            << Path
            << '\n';

        cgltf_free(Data);
        Shutdown();
        return false;
    }

    Result = cgltf_validate(Data);

    if (Result != cgltf_result_success)
    {
        std::cerr
            << "Entity model validation failed: "
            << Path
            << '\n';

        cgltf_free(Data);
        Shutdown();
        return false;
    }

    BoundsMin = glm::vec3(
        std::numeric_limits<float>::infinity()
    );

    BoundsMax = glm::vec3(
        -std::numeric_limits<float>::infinity()
    );

    for (cgltf_size NodeIndex = 0;
         NodeIndex < Data->nodes_count;
         ++NodeIndex)
    {
        const cgltf_node& Node = Data->nodes[NodeIndex];

        if (Node.mesh == nullptr)
            continue;

        const glm::mat4 NodeMatrix =
            NodeWorldMatrix(Node);

        const glm::mat3 NormalMatrix =
            glm::transpose(
                glm::inverse(glm::mat3(NodeMatrix))
            );

        for (cgltf_size PrimitiveIndex = 0;
             PrimitiveIndex < Node.mesh->primitives_count;
             ++PrimitiveIndex)
        {
            const cgltf_primitive& SourcePrimitive =
                Node.mesh->primitives[PrimitiveIndex];

            if (
                SourcePrimitive.type !=
                cgltf_primitive_type_triangles
            )
            {
                continue;
            }

            const cgltf_accessor* Positions =
                FindAttribute(
                    SourcePrimitive,
                    cgltf_attribute_type_position
                );

            if (Positions == nullptr)
                continue;

            const cgltf_accessor* Normals =
                FindAttribute(
                    SourcePrimitive,
                    cgltf_attribute_type_normal
                );

            const cgltf_accessor* TexCoords =
                FindAttribute(
                    SourcePrimitive,
                    cgltf_attribute_type_texcoord
                );

            std::vector<Vertex> Vertices(
                static_cast<std::size_t>(Positions->count)
            );

            for (cgltf_size I = 0;
                 I < Positions->count;
                 ++I)
            {
                float P[3] = {};
                float N[3] = {0.0f, 1.0f, 0.0f};
                float UV[2] = {};

                cgltf_accessor_read_float(
                    Positions,
                    I,
                    P,
                    3
                );

                if (Normals != nullptr)
                {
                    cgltf_accessor_read_float(
                        Normals,
                        I,
                        N,
                        3
                    );
                }

                if (TexCoords != nullptr)
                {
                    cgltf_accessor_read_float(
                        TexCoords,
                        I,
                        UV,
                        2
                    );
                }

                const glm::vec3 Position =
                    glm::vec3(
                        NodeMatrix *
                        glm::vec4(P[0], P[1], P[2], 1.0f)
                    );

                glm::vec3 Normal =
                    NormalMatrix *
                    glm::vec3(N[0], N[1], N[2]);

                const float NormalLength =
                    glm::length(Normal);

                if (NormalLength > 0.0001f)
                    Normal /= NormalLength;
                else
                    Normal = {0.0f, 1.0f, 0.0f};

                Vertex& Target =
                    Vertices[static_cast<std::size_t>(I)];

                Target.Position = Position;
                Target.Normal = Normal;
                Target.TexCoord = {UV[0], UV[1]};

                BoundsMin = glm::min(
                    BoundsMin,
                    Position
                );

                BoundsMax = glm::max(
                    BoundsMax,
                    Position
                );
            }

            std::vector<std::uint32_t> Indices;

            if (SourcePrimitive.indices != nullptr)
            {
                Indices.resize(
                    static_cast<std::size_t>(
                        SourcePrimitive.indices->count
                    )
                );

                for (cgltf_size I = 0;
                     I < SourcePrimitive.indices->count;
                     ++I)
                {
                    Indices[static_cast<std::size_t>(I)] =
                        static_cast<std::uint32_t>(
                            cgltf_accessor_read_index(
                                SourcePrimitive.indices,
                                I
                            )
                        );
                }
            }
            else
            {
                Indices.resize(Vertices.size());

                for (std::size_t I = 0;
                     I < Indices.size();
                     ++I)
                {
                    Indices[I] =
                        static_cast<std::uint32_t>(I);
                }
            }

            if (Vertices.empty() || Indices.empty())
                continue;

            Primitive TargetPrimitive;

            glGenVertexArrays(
                1,
                &TargetPrimitive.VertexArray
            );

            glBindVertexArray(
                TargetPrimitive.VertexArray
            );

            glGenBuffers(
                1,
                &TargetPrimitive.VertexBuffer
            );

            glBindBuffer(
                GL_ARRAY_BUFFER,
                TargetPrimitive.VertexBuffer
            );

            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(
                    Vertices.size() * sizeof(Vertex)
                ),
                Vertices.data(),
                GL_STATIC_DRAW
            );

            glGenBuffers(
                1,
                &TargetPrimitive.IndexBuffer
            );

            glBindBuffer(
                GL_ELEMENT_ARRAY_BUFFER,
                TargetPrimitive.IndexBuffer
            );

            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(
                    Indices.size() *
                    sizeof(std::uint32_t)
                ),
                Indices.data(),
                GL_STATIC_DRAW
            );

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(
                    offsetof(Vertex, Position)
                )
            );

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                1,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(
                    offsetof(Vertex, Normal)
                )
            );

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(
                2,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(Vertex),
                reinterpret_cast<void*>(
                    offsetof(Vertex, TexCoord)
                )
            );

            TargetPrimitive.IndexCount =
                static_cast<GLsizei>(Indices.size());

            if (SourcePrimitive.material != nullptr)
            {
                const cgltf_material& Material =
                    *SourcePrimitive.material;

                if (Material.has_pbr_metallic_roughness)
                {
                    const cgltf_pbr_metallic_roughness& Pbr =
                        Material.pbr_metallic_roughness;

                    TargetPrimitive.BaseColor = {
                        Pbr.base_color_factor[0],
                        Pbr.base_color_factor[1],
                        Pbr.base_color_factor[2],
                        Pbr.base_color_factor[3]
                    };

                    const cgltf_texture* Texture =
                        Pbr.base_color_texture.texture;

                    if (
                        Texture != nullptr &&
                        Texture->image != nullptr
                    )
                    {
                        const cgltf_image& Image =
                            *Texture->image;

                        if (
                            Image.buffer_view != nullptr &&
                            Image.buffer_view->buffer != nullptr &&
                            Image.buffer_view->buffer->data != nullptr
                        )
                        {
                            const cgltf_buffer_view& View =
                                *Image.buffer_view;

                            const unsigned char* ImageData =
                                static_cast<const unsigned char*>(
                                    View.buffer->data
                                ) +
                                View.offset;

                            TargetPrimitive.Texture =
                                CreateTexture(
                                    ImageData,
                                    static_cast<std::size_t>(
                                        View.size
                                    )
                                );
                        }
                        else if (
                            Image.uri != nullptr &&
                            Image.uri[0] != '\0'
                        )
                        {
                            const std::filesystem::path ImagePath =
                                ModelDirectory /
                                std::filesystem::path(Image.uri);

                            std::ifstream ImageFile(
                                ImagePath,
                                std::ios::binary
                            );

                            if (ImageFile.is_open())
                            {
                                std::vector<unsigned char> Bytes(
                                    std::istreambuf_iterator<char>(ImageFile),
                                    std::istreambuf_iterator<char>()
                                );

                                if (!Bytes.empty())
                                {
                                    TargetPrimitive.Texture =
                                        CreateTexture(
                                            Bytes.data(),
                                            Bytes.size()
                                        );
                                }
                            }
                        }
                    }
                }
            }

            if (TargetPrimitive.Texture == 0)
            {
                TargetPrimitive.Texture =
                    CreateWhiteTexture();
            }

            glBindVertexArray(0);

            Primitives.push_back(
                TargetPrimitive
            );
        }
    }

    cgltf_free(Data);

    if (Primitives.empty())
    {
        std::cerr
            << "Entity model contains no renderable mesh: "
            << Path
            << '\n';

        Shutdown();
        return false;
    }

    Center = (BoundsMin + BoundsMax) * 0.5f;
    GroundY = BoundsMin.y;

    const float Height =
        std::max(
            BoundsMax.y - BoundsMin.y,
            0.001f
        );

    Scale =
        std::max(TargetHeight, 0.1f) /
        Height;

    Ready = true;
    return true;
}

void EntityModel::Shutdown()
{
    for (Primitive& PrimitiveData : Primitives)
    {
        if (PrimitiveData.Texture != 0)
            glDeleteTextures(1, &PrimitiveData.Texture);

        if (PrimitiveData.IndexBuffer != 0)
            glDeleteBuffers(1, &PrimitiveData.IndexBuffer);

        if (PrimitiveData.VertexBuffer != 0)
            glDeleteBuffers(1, &PrimitiveData.VertexBuffer);

        if (PrimitiveData.VertexArray != 0)
            glDeleteVertexArrays(1, &PrimitiveData.VertexArray);
    }

    Primitives.clear();

    if (Program != 0)
        glDeleteProgram(Program);

    Program = 0;
    Ready = false;
}

bool EntityModel::IsReady() const
{
    return Ready;
}

void EntityModel::Draw(
    const glm::mat4& View,
    const glm::mat4& Projection,
    const glm::vec3& CameraPosition,
    const glm::vec3& Position,
    const glm::vec3& Forward,
    const glm::vec3& VisualScale,
    const std::array<glm::vec4, 8>& LightPositions,
    const std::array<glm::vec4, 8>& LightColors,
    int LightCount
) const
{
    if (!Ready || Program == 0)
        return;

    glm::vec3 Direction = Forward;
    Direction.y = 0.0f;

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, 0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const float Yaw =
        std::atan2(Direction.x, Direction.z);

    const glm::mat4 Model =
        glm::translate(
            glm::mat4(1.0f),
            Position
        ) *
        glm::rotate(
            glm::mat4(1.0f),
            Yaw,
            glm::vec3{0.0f, 1.0f, 0.0f}
        ) *
        glm::scale(
            glm::mat4(1.0f),
            VisualScale
        ) *
        glm::scale(
            glm::mat4(1.0f),
            glm::vec3{Scale}
        ) *
        glm::translate(
            glm::mat4(1.0f),
            glm::vec3{
                -Center.x,
                -GroundY,
                -Center.z
            }
        );

    glUseProgram(Program);

    glUniformMatrix4fv(
        ModelLocation,
        1,
        GL_FALSE,
        glm::value_ptr(Model)
    );

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

    const int Count =
        std::clamp(LightCount, 0, 8);

    glUniform1i(
        LightCountLocation,
        Count
    );

    if (Count > 0)
    {
        glUniform4fv(
            LightPositionLocation,
            Count,
            glm::value_ptr(LightPositions[0])
        );

        glUniform4fv(
            LightColorLocation,
            Count,
            glm::value_ptr(LightColors[0])
        );
    }

    glUniform1i(TextureLocation, 0);

    glEnable(GL_BLEND);
    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    for (const Primitive& PrimitiveData : Primitives)
    {
        glUniform4fv(
            BaseColorLocation,
            1,
            glm::value_ptr(PrimitiveData.BaseColor)
        );

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            PrimitiveData.Texture
        );

        glBindVertexArray(
            PrimitiveData.VertexArray
        );

        glDrawElements(
            GL_TRIANGLES,
            PrimitiveData.IndexCount,
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}
