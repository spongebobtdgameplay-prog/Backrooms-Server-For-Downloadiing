from pathlib import Path
import re


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one match, got {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def regex_once(path, pattern, replacement):
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    new_text, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise RuntimeError(f"{path}: regex expected one match, got {count}")
    p.write_text(new_text, encoding="utf-8", newline="\n")


replace_once(
    "src/Rendering/Renderer.h",
    "    std::array<glm::vec4, 8> ActiveLightColors{};\n    int ActiveLightCount = 0;\n",
    "    std::array<glm::vec4, 8> ActiveLightColors{};\n    int ActiveLightCount = 0;\n    float RenderTime = 0.0f;\n"
)

replace_once(
    "src/Rendering/Renderer.cpp",
    "void Renderer::SetLights(\n    const std::vector<LightPoint>& Lights,\n    const glm::vec3& ViewerPosition,\n    float Time\n)\n{\n    struct Candidate\n",
    "void Renderer::SetLights(\n    const std::vector<LightPoint>& Lights,\n    const glm::vec3& ViewerPosition,\n    float Time\n)\n{\n    RenderTime = Time;\n\n    struct Candidate\n"
)

entity_draw = r'''bool Renderer::HasEntityModels() const
{
    return true;
}

void Renderer::DrawEntity(
    const glm::vec3& Position,
    const glm::vec3& Forward,
    bool DemonForm,
    bool PreviousDemonForm,
    float ShiftProgress
)
{
    glm::vec3 Direction = Forward;
    Direction.y = 0.0f;

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, 0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const float Yaw = std::atan2(Direction.x, Direction.z);
    const float GaitSpeed = DemonForm ? 10.2f : 8.4f;
    const float Gait = std::sin(RenderTime * GaitSpeed);
    const float OppositeGait = std::sin(RenderTime * GaitSpeed + 3.14159265f);
    const float Bob = std::abs(std::sin(RenderTime * GaitSpeed)) * 0.045f;
    const float Shift = std::clamp(ShiftProgress, 0.0f, 1.0f);
    const float Hunch = DemonForm ? 0.26f : 0.15f;

    const glm::vec3 BodyColor = DemonForm
        ? glm::vec3{0.105f, 0.040f, 0.032f}
        : glm::vec3{0.050f, 0.052f, 0.046f};
    const glm::vec3 LimbColor = DemonForm
        ? glm::vec3{0.075f, 0.026f, 0.022f}
        : glm::vec3{0.035f, 0.038f, 0.034f};
    const glm::vec3 FaceColor = DemonForm
        ? glm::vec3{0.125f, 0.105f, 0.082f}
        : glm::vec3{0.090f, 0.092f, 0.080f};
    const glm::vec3 EyeColor = DemonForm
        ? glm::vec3{0.70f, 0.055f, 0.025f}
        : glm::vec3{0.32f, 0.31f, 0.19f};

    const glm::mat4 Root =
        glm::translate(
            glm::mat4(1.0f),
            Position + glm::vec3{0.0f, Bob, 0.0f}
        ) *
        glm::rotate(
            glm::mat4(1.0f),
            Yaw,
            glm::vec3{0.0f, 1.0f, 0.0f}
        );

    auto PushPart = [&](const glm::vec3& LocalPosition,
                        const glm::vec3& Size,
                        float RotateX,
                        float RotateZ,
                        const glm::vec3& Color,
                        const glm::vec3& Emissive,
                        float Roughness)
    {
        InstanceData Instance;
        Instance.Model =
            Root *
            glm::translate(glm::mat4(1.0f), LocalPosition) *
            glm::rotate(glm::mat4(1.0f), RotateX, glm::vec3{1.0f, 0.0f, 0.0f}) *
            glm::rotate(glm::mat4(1.0f), RotateZ, glm::vec3{0.0f, 0.0f, 1.0f}) *
            glm::scale(glm::mat4(1.0f), Size);
        Instance.Color = glm::vec4(Color, 1.0f);
        Instance.EmissiveRoughness = glm::vec4(Emissive, Roughness);
        Instance.SurfaceData = glm::vec4(0.0f);
        Instances.push_back(Instance);
    };

    const float BodyLean = Hunch + Gait * 0.025f;
    const float ArmSwing = Gait * (DemonForm ? 0.72f : 0.58f);
    const float LegSwing = Gait * (DemonForm ? 0.55f : 0.44f);

    PushPart({0.0f, 1.30f, 0.0f}, {0.42f, 0.72f, 0.28f}, BodyLean, 0.0f, BodyColor, {0.0f, 0.0f, 0.0f}, 0.94f);
    PushPart({0.0f, 0.83f, -0.025f}, {0.33f, 0.30f, 0.24f}, BodyLean * 0.45f, 0.0f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.96f);
    PushPart({0.0f, 1.78f, -0.035f}, {0.14f, 0.20f, 0.14f}, BodyLean * 0.70f, 0.0f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.96f);
    PushPart({0.0f, 2.04f, 0.02f}, {0.29f, 0.34f, 0.30f}, BodyLean * 0.9f - Gait * 0.02f, Gait * 0.025f, FaceColor, {0.0f, 0.0f, 0.0f}, 0.92f);

    PushPart({-0.43f, 1.34f, 0.0f}, {0.14f, 0.82f, 0.14f}, ArmSwing, -0.08f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.97f);
    PushPart({0.43f, 1.34f, 0.0f}, {0.14f, 0.82f, 0.14f}, OppositeGait * (DemonForm ? 0.72f : 0.58f), 0.08f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.97f);
    PushPart({-0.50f, 0.83f, 0.02f}, {0.13f, 0.48f, 0.13f}, ArmSwing * 0.45f + 0.18f, -0.04f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.98f);
    PushPart({0.50f, 0.83f, 0.02f}, {0.13f, 0.48f, 0.13f}, OppositeGait * 0.32f + 0.18f, 0.04f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.98f);

    PushPart({-0.17f, 0.44f, 0.0f}, {0.17f, 0.78f, 0.18f}, LegSwing, 0.0f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.98f);
    PushPart({0.17f, 0.44f, 0.0f}, {0.17f, 0.78f, 0.18f}, OppositeGait * (DemonForm ? 0.55f : 0.44f), 0.0f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.98f);
    PushPart({-0.17f, 0.08f, 0.10f}, {0.19f, 0.12f, 0.34f}, 0.0f, 0.0f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.99f);
    PushPart({0.17f, 0.08f, 0.10f}, {0.19f, 0.12f, 0.34f}, 0.0f, 0.0f, LimbColor, {0.0f, 0.0f, 0.0f}, 0.99f);

    const float EyeLift = 2.075f + (1.0f - Shift) * 0.025f;
    PushPart({-0.075f, EyeLift, 0.175f}, {0.045f, 0.035f, 0.025f}, 0.0f, 0.0f, EyeColor, EyeColor * 0.45f, 0.55f);
    PushPart({0.075f, EyeLift, 0.175f}, {0.045f, 0.035f, 0.025f}, 0.0f, 0.0f, EyeColor, EyeColor * 0.45f, 0.55f);
}

void Renderer::DrawCrosshair'''

regex_once(
    "src/Rendering/Renderer.cpp",
    r"bool Renderer::HasEntityModels\(\) const\n\{.*?\n\}\n\nvoid Renderer::DrawCrosshair",
    entity_draw
)

replace_once(
    "src/Entity/Entity.cpp",
    "    ReleaseGraceTimer = 5.0f;\n",
    "    ReleaseGraceTimer = 2.2f;\n"
)
replace_once(
    "src/Entity/Entity.cpp",
    "    float MoveSpeed =\n        1.65f + ChaseBlend * 1.75f;\n\n    if (ReleaseGraceTimer > 0.0f)\n        MoveSpeed = std::min(MoveSpeed, 1.15f);\n\n    if (DemonForm && ReleaseGraceTimer <= 0.0f)\n        MoveSpeed += 0.28f;\n",
    "    float MoveSpeed =\n        2.15f + ChaseBlend * 2.35f;\n\n    if (ReleaseGraceTimer > 0.0f)\n        MoveSpeed = std::min(MoveSpeed, 1.55f);\n\n    if (DemonForm && ReleaseGraceTimer <= 0.0f)\n        MoveSpeed += 0.42f;\n"
)
replace_once(
    "src/Entity/Entity.h",
    "    float RepathInterval = 0.34f;\n",
    "    float RepathInterval = 0.18f;\n"
)

replace_once(
    "src/Audio/AudioSystem.cpp",
    "    EntityLaugh = LoadSound(\n        \"assets/audio/entity-laugh.wav\",\n        false,\n        true,\n        0.46f\n    );\n\n    if (EntityLaugh != nullptr)\n    {\n        ma_sound_set_min_distance(EntityLaugh, 2.0f);\n        ma_sound_set_max_distance(EntityLaugh, 78.0f);\n        ma_sound_set_rolloff(EntityLaugh, 0.48f);\n    }\n",
    "    EntityLaugh = nullptr;\n"
)
regex_once(
    "src/Audio/AudioSystem.cpp",
    r"            if \(\n                EntityLaugh != nullptr &&\n                EntityCueIndex % 3 == 2\n            \)\n            \{.*?            \}\n            else\n            \{\n                RestartSpatial\(\n                    EntityMetal,\n                    EntityPosition \+ glm::vec3\{0\.0f, 0\.8f, 0\.0f\},\n                    0\.42f \+ Threat \* 0\.32f,\n                    Pitch\n                \);\n            \}\n",
    "            RestartSpatial(\n                EntityMetal,\n                EntityPosition + glm::vec3{0.0f, 0.8f, 0.0f},\n                0.34f + Threat * 0.30f,\n                0.78f + Threat * 0.08f + static_cast<float>(EntityCueIndex % 3) * 0.025f\n            );\n"
)
regex_once(
    "src/Audio/AudioSystem.cpp",
    r"\n    if \(DemonForm\)\n    \{\n        RestartSpatial\(\n            EntityLaugh,.*?\n    \}\n\}",
    "\n}\n"
)

replace_once(
    "src/Rendering/MapOverlay.cpp",
    "            Line(A, B, Detailed ? 6 : 5, RouteOuter);\n            Line(A, B, 3, RouteColor);\n",
    "            Line(A, B, Detailed ? 6 : 3, RouteOuter);\n            Line(A, B, Detailed ? 3 : 1, RouteColor);\n"
)
replace_once(
    "src/Rendering/MapOverlay.cpp",
    "                FillCircle(Position.x, Position.y, 4, Ink);\n                FillCircle(Position.x, Position.y, 3, Glow);\n                ClipRect(Position.x - 1, Position.y - 1, 2, 2, Ink);\n",
    "                FillCircle(Position.x, Position.y, 3, Ink);\n                FillCircle(Position.x, Position.y, 2, Glow);\n"
)
replace_once(
    "src/Rendering/MapOverlay.cpp",
    "                ClipRect(Position.x - 4, Position.y - 4, 9, 9, Ink);\n                ClipRect(Position.x - 3, Position.y - 3, 7, 7, Color);\n                ClipRect(Position.x + 1, Position.y - 2, 1, 5, Ink);\n",
    "                ClipRect(Position.x - 3, Position.y - 3, 7, 7, Ink);\n                ClipRect(Position.x - 2, Position.y - 2, 5, 5, Color);\n                ClipRect(Position.x, Position.y - 1, 1, 3, Ink);\n"
)
regex_once(
    "src/Rendering/MapOverlay.cpp",
    r"            if \(!Detailed\)\n            \{\n                const int Radius = 4;\n                const glm::ivec2 Top\{Position.x, Position.y - Radius\};\n                const glm::ivec2 LeftPoint\{Position.x - Radius, Position.y\};\n                const glm::ivec2 Bottom\{Position.x, Position.y \+ Radius\};\n                const glm::ivec2 RightPoint\{Position.x \+ Radius, Position.y\};\n                FillTriangle\(Top, LeftPoint, RightPoint, Threat\);\n                FillTriangle\(Bottom, LeftPoint, RightPoint, Threat\);\n                ClipRect\(Position.x, Position.y, 1, 1, Yellow\);\n            \}",
    "            if (!Detailed)\n            {\n                ClipRect(Position.x - 3, Position.y - 3, 7, 7, Ink);\n                ClipRect(Position.x - 2, Position.y - 2, 5, 5, Threat);\n            }"
)
replace_once(
    "src/Rendering/MapOverlay.cpp",
    "        const int Pulse = Detailed\n            ? 9 + static_cast<int>((std::sin(Time * 5.0f) + 1.0f) * 2.0f)\n            : 5 + static_cast<int>((std::sin(Time * 5.0f) + 1.0f) * 1.0f);\n        Ring(Position.x, Position.y, Pulse + 3, 2, Ink);\n        Ring(Position.x, Position.y, Pulse, 2, RouteColor);\n        Line({Position.x - 5, Position.y}, {Position.x + 5, Position.y}, 2, RouteColor);\n        Line({Position.x, Position.y - 5}, {Position.x, Position.y + 5}, 2, RouteColor);\n",
    "        if (!Detailed)\n        {\n            Ring(Position.x, Position.y, 4, 1, Ink);\n            ClipRect(Position.x - 1, Position.y - 1, 3, 3, RouteColor);\n        }\n        else\n        {\n            const int Pulse = 9 + static_cast<int>((std::sin(Time * 5.0f) + 1.0f) * 2.0f);\n            Ring(Position.x, Position.y, Pulse + 3, 2, Ink);\n            Ring(Position.x, Position.y, Pulse, 2, RouteColor);\n            Line({Position.x - 5, Position.y}, {Position.x + 5, Position.y}, 2, RouteColor);\n            Line({Position.x, Position.y - 5}, {Position.x, Position.y + 5}, 2, RouteColor);\n        }\n"
)
replace_once(
    "src/Rendering/MapOverlay.cpp",
    "    const int ArrowLength = Detailed ? 14 : 7;\n    const int ArrowWidth = Detailed ? 8 : 4;\n    const float TailLength = Detailed ? 5.0f : 2.0f;\n",
    "    const int ArrowLength = Detailed ? 14 : 5;\n    const int ArrowWidth = Detailed ? 8 : 3;\n    const float TailLength = Detailed ? 5.0f : 1.0f;\n"
)
replace_once(
    "src/Rendering/MapOverlay.cpp",
    "    FillTriangle(\n        {Tip.x + 2, Tip.y + 2},\n        {Left.x + 2, Left.y + 2},\n        {RightPoint.x + 2, RightPoint.y + 2},\n        Ink\n    );\n    FillTriangle(Tip, Left, RightPoint, {0.98f, 0.97f, 0.86f});\n    FillCircle(PlayerScreen.x, PlayerScreen.y, Detailed ? 3 : 2, RouteColor);\n",
    "    if (Detailed)\n    {\n        FillTriangle(\n            {Tip.x + 2, Tip.y + 2},\n            {Left.x + 2, Left.y + 2},\n            {RightPoint.x + 2, RightPoint.y + 2},\n            Ink\n        );\n    }\n    FillTriangle(Tip, Left, RightPoint, {0.98f, 0.97f, 0.86f});\n    FillCircle(PlayerScreen.x, PlayerScreen.y, Detailed ? 3 : 1, RouteColor);\n"
)

for path in [
    "src/Core/Version.h",
    "CMakeLists.txt",
    "src/Platform/Windows/Backrooms.rc",
    "update/release_notes.txt",
]:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    if "0.3.33" not in text:
        raise RuntimeError(f"{path}: missing 0.3.33")
    text = text.replace("0.3.33", "0.3.34")
    p.write_text(text, encoding="utf-8", newline="\n")

notes = Path("update/release_notes.txt")
notes.write_text(
    "V0.3.34 replaces the static clown-like entity presentation with a dark animated humanoid silhouette that visibly walks, leans, bobs, and swings its limbs while chasing. The repeating laugh cue was removed, entity pursuit is faster and repaths more often, release grace is shorter, and the gameplay minimap now uses thinner route lines plus smaller, simpler breaker, exit, threat, waypoint, and player markers. Edge clipping remains intact.\n",
    encoding="utf-8",
    newline="\n"
)
