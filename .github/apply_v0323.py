from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text()
    if old not in text:
        raise RuntimeError(f"pattern not found in {path}: {old[:100]!r}")
    p.write_text(text.replace(old, new, 1))


replace_once(
    "src/Entity/Entity.cpp",
    "    ShiftTimer = 7.0f;\n    ShiftProgress = 1.0f;\n\n    Active = false;",
    "    ShiftTimer = 7.0f;\n    ShiftProgress = 1.0f;\n    ReleaseGraceTimer = 0.0f;\n    EncounterAge = 0.0f;\n\n    Active = false;"
)

replace_once(
    "src/Entity/Entity.cpp",
    "void Entity::Release()\n{\n    Active = true;\n    RepathTimer = 0.0f;\n}",
    "void Entity::Release()\n{\n    Active = true;\n    RepathTimer = 0.0f;\n    ReleaseGraceTimer = 5.0f;\n    EncounterAge = 0.0f;\n}"
)

replace_once(
    "src/Entity/Entity.cpp",
    "    if (!Active)\n        return false;\n\n    RepathTimer -= DeltaTime;",
    "    if (!Active)\n        return false;\n\n    EncounterAge += std::max(DeltaTime, 0.0f);\n    ReleaseGraceTimer = std::max(\n        0.0f,\n        ReleaseGraceTimer - DeltaTime\n    );\n\n    const float DistanceBeforeMove =\n        DistanceTo(PlayerPosition);\n\n    const float ChaseBlend = std::clamp(\n        1.0f - (DistanceBeforeMove - 8.0f) / 22.0f,\n        0.0f,\n        1.0f\n    );\n\n    float MoveSpeed =\n        1.65f + ChaseBlend * 1.75f;\n\n    if (ReleaseGraceTimer > 0.0f)\n        MoveSpeed = std::min(MoveSpeed, 1.15f);\n\n    if (DemonForm && ReleaseGraceTimer <= 0.0f)\n        MoveSpeed += 0.28f;\n\n    RepathTimer -= DeltaTime;"
)

replace_once(
    "src/Entity/Entity.cpp",
    "                std::min(Speed * DeltaTime, Distance);",
    "                std::min(MoveSpeed * DeltaTime, Distance);"
)

replace_once(
    "src/Entity/Entity.cpp",
    "    return DistanceTo(PlayerPosition) < 0.9f;",
    "    return\n        ReleaseGraceTimer <= 0.0f &&\n        DistanceTo(PlayerPosition) < 0.9f;"
)

mount_function = r'''bool Game::TryMountBreaker(
    const glm::vec3& CellCenter,
    int Variant,
    Breaker& Result
) const
{
    const int X = std::clamp(
        static_cast<int>(std::round(CellCenter.x / World.CellSize)),
        0,
        World.Columns - 1
    );

    const int Z = std::clamp(
        static_cast<int>(std::round(CellCenter.z / World.CellSize)),
        0,
        World.Rows - 1
    );

    const MazeCell& Cell = World.Cell(X, Z);

    const int StartDirection =
        static_cast<int>(
            (Seed + static_cast<uint32_t>(Variant * 2654435761u)) % 4u
        );

    for (int Offset = 0; Offset < 4; ++Offset)
    {
        const int DirectionIndex =
            (StartDirection + Offset) % 4;

        if (!Cell.Walls[static_cast<std::size_t>(DirectionIndex)])
            continue;

        glm::vec3 WallCenter = CellCenter;
        glm::vec3 Forward{0.0f};

        if (DirectionIndex == 0)
        {
            WallCenter.z -= World.CellSize * 0.5f;
            Forward = {0.0f, 0.0f, 1.0f};
        }
        else if (DirectionIndex == 1)
        {
            WallCenter.x += World.CellSize * 0.5f;
            Forward = {-1.0f, 0.0f, 0.0f};
        }
        else if (DirectionIndex == 2)
        {
            WallCenter.z += World.CellSize * 0.5f;
            Forward = {0.0f, 0.0f, -1.0f};
        }
        else
        {
            WallCenter.x -= World.CellSize * 0.5f;
            Forward = {1.0f, 0.0f, 0.0f};
        }

        const glm::vec3 Right{
            Forward.z,
            0.0f,
            -Forward.x
        };

        const uint32_t Hash =
            Seed ^
            static_cast<uint32_t>(Variant * 2246822519u) ^
            static_cast<uint32_t>((X + 1) * 3266489917u) ^
            static_cast<uint32_t>((Z + 1) * 668265263u);

        const float Along =
            (static_cast<float>(Hash % 1000u) / 999.0f - 0.5f) *
            1.45f;

        Result.Position =
            WallCenter +
            Forward * (World.WallThickness * 0.5f + 0.11f) +
            Right * Along;

        Result.Position.y = 0.88f;
        Result.Forward = Forward;
        Result.Active = false;
        return true;
    }

    return false;
}

'''

replace_once(
    "src/Game/Game.cpp",
    "void Game::Reset()\n{",
    mount_function + "void Game::Reset()\n{"
)

replace_once(
    "src/Game/Game.cpp",
    "    GamePlayer.Reset({0.0f, 1.65f, 0.0f});\n\n    State = {};",
    "    GamePlayer.Reset({0.0f, 1.65f, 0.0f});\n    PreviousPlayerPosition = GamePlayer.Position();\n    FootstepDistance = 0.0f;\n\n    State = {};"
)

replace_once(
    "src/Game/Game.cpp",
    "    for (int I = 0; I < 3; ++I)\n    {\n        Breakers.push_back({\n            Pick(24.0f),\n            false\n        });\n    }",
    "    for (int I = 0; I < 3; ++I)\n    {\n        Breaker Mounted;\n        bool MountedSuccessfully = false;\n\n        for (int Attempt = 0; Attempt < 36; ++Attempt)\n        {\n            const glm::vec3 Candidate = Pick(24.0f);\n\n            if (TryMountBreaker(\n                    Candidate,\n                    I * 41 + Attempt,\n                    Mounted\n                ))\n            {\n                MountedSuccessfully = true;\n                break;\n            }\n        }\n\n        if (!MountedSuccessfully)\n            continue;\n\n        Breakers.push_back(Mounted);\n        World.Colliders.push_back(\n            BreakerBounds(Breakers.back())\n        );\n    }"
)

replace_once(
    "src/Game/Game.cpp",
    "AABB Game::BreakerBounds(const Breaker& BreakerData) const\n{\n    return {\n        BreakerData.Position + glm::vec3{-0.42f, 0.72f, -0.3f},\n        BreakerData.Position + glm::vec3{0.42f, 1.88f, 0.3f}\n    };\n}",
    "AABB Game::BreakerBounds(const Breaker& BreakerData) const\n{\n    const glm::vec3 Forward = BreakerData.Forward;\n    const glm::vec3 Right{\n        Forward.z,\n        0.0f,\n        -Forward.x\n    };\n\n    const float HalfX =\n        std::abs(Right.x) * 0.40f +\n        std::abs(Forward.x) * 0.18f;\n\n    const float HalfZ =\n        std::abs(Right.z) * 0.40f +\n        std::abs(Forward.z) * 0.18f;\n\n    return {\n        {\n            BreakerData.Position.x - HalfX,\n            BreakerData.Position.y,\n            BreakerData.Position.z - HalfZ\n        },\n        {\n            BreakerData.Position.x + HalfX,\n            BreakerData.Position.y + 1.05f,\n            BreakerData.Position.z + HalfZ\n        }\n    };\n}"
)

replace_once(
    "src/Game/Game.cpp",
    "        Target.Active = true;\n        State.ActivateBreaker();",
    "        Target.Active = true;\n        Audio.PlayBreaker(Target.Position);\n        State.ActivateBreaker();"
)

replace_once(
    "src/Game/Game.cpp",
    "    GamePlayer.Update(\n        DeltaTime,\n        World.Colliders,\n        MouseCaptured\n    );\n\n    UpdateInteraction();",
    "    GamePlayer.Update(\n        DeltaTime,\n        World.Colliders,\n        MouseCaptured\n    );\n\n    glm::vec3 PlayerTravel =\n        GamePlayer.Position() - PreviousPlayerPosition;\n    PlayerTravel.y = 0.0f;\n\n    const float TravelDistance = glm::length(PlayerTravel);\n\n    if (TravelDistance > 0.0001f)\n    {\n        FootstepDistance += TravelDistance;\n\n        while (FootstepDistance >= 1.28f)\n        {\n            FootstepDistance -= 1.28f;\n            Audio.PlayFootstep(GamePlayer.Position());\n        }\n    }\n\n    PreviousPlayerPosition = GamePlayer.Position();\n\n    UpdateInteraction();"
)

replace_once(
    "src/Game/Game.cpp",
    "    float EntityDistance =\n        std::numeric_limits<float>::infinity();\n\n    if (State.EntityReleased)",
    "    if (State.EntityReleased)"
)

replace_once(
    "src/Game/Game.cpp",
    "        EntityDistance =\n            Hunter.DistanceTo(GamePlayer.Position());\n\n        if (Hunter.ConsumeShifted())",
    "        if (Hunter.ConsumeShifted())"
)

replace_once(
    "src/Game/Game.cpp",
    "            Audio.PlayShift(\n                Hunter.IsDemonForm()\n            );",
    "            Audio.PlayShift(\n                Hunter.IsDemonForm(),\n                Hunter.Position()\n            );"
)

replace_once(
    "src/Game/Game.cpp",
    "    Audio.Update(EntityDistance);\n    UpdateTitle();",
    "    Audio.Update(\n        DeltaTime,\n        GamePlayer.Position(),\n        GamePlayer.Forward(),\n        Hunter.Position(),\n        Hunter.IsActive()\n    );\n    UpdateTitle();"
)

old_breaker_render = r'''    for (const Breaker& BreakerData : Breakers)
    {
        Boxes.push_back({
            BreakerData.Position + glm::vec3{0.0f, 1.35f, 0.0f},
            {0.7f, 1.0f, 0.18f},
            {0.0409f, 0.0382f, 0.0203f},
            {0.0f, 0.0f, 0.0f},
            0.8f
        });

        Boxes.push_back({
            BreakerData.Position + glm::vec3{0.0f, 1.35f, -0.14f},
            {0.18f, 0.34f, 0.1f},
            BreakerData.Active
                ? glm::vec3{0.0437f, 0.4564f, 0.0723f}
                : glm::vec3{0.4735f, 0.0513f, 0.0273f},
            BreakerData.Active
                ? glm::vec3{0.00212f, 0.04667f, 0.00518f}
                : glm::vec3{0.04231f, 0.00273f, 0.00121f},
            0.7f
        });
    }
'''

new_breaker_render = r'''    if (!GameRenderer.HasBreakerModel())
    {
        for (const Breaker& BreakerData : Breakers)
        {
            const bool FacingX =
                std::abs(BreakerData.Forward.x) > 0.5f;

            Boxes.push_back({
                BreakerData.Position + glm::vec3{0.0f, 0.525f, 0.0f},
                FacingX
                    ? glm::vec3{0.18f, 1.05f, 0.76f}
                    : glm::vec3{0.76f, 1.05f, 0.18f},
                {0.16f, 0.17f, 0.15f},
                {0.0f, 0.0f, 0.0f},
                0.88f,
                static_cast<int>(SurfaceMaterial::Fixture)
            });
        }
    }
'''

replace_once(
    "src/Game/Game.cpp",
    old_breaker_render,
    new_breaker_render
)

replace_once(
    "src/Game/Game.cpp",
    "    GameRenderer.DrawBoxes(Dynamic);\n\n    if (\n        Hunter.IsActive() &&",
    "    GameRenderer.DrawBoxes(Dynamic);\n\n    if (GameRenderer.HasBreakerModel())\n    {\n        for (const Breaker& BreakerData : Breakers)\n        {\n            GameRenderer.DrawBreaker(\n                BreakerData.Position,\n                BreakerData.Forward\n            );\n        }\n    }\n\n    if (\n        Hunter.IsActive() &&"
)

replace_once(
    "src/Rendering/Renderer.cpp",
    "    GhostEntityModel.Load(\n        \"assets/models/entity-ghost.glb\",\n        2.18f\n    );",
    "    BreakerModel.Load(\n        \"assets/models/power_box_01/power_box_01_1k.gltf\",\n        1.05f\n    );\n\n    GhostEntityModel.Load(\n        \"assets/models/entity-ghost.glb\",\n        2.18f\n    );"
)

replace_once(
    "src/Rendering/Renderer.cpp",
    "    GhostEntityModel.Shutdown();\n    DemonEntityModel.Shutdown();",
    "    BreakerModel.Shutdown();\n    GhostEntityModel.Shutdown();\n    DemonEntityModel.Shutdown();"
)

breaker_renderer = r'''bool Renderer::HasBreakerModel() const
{
    return BreakerModel.IsReady();
}

void Renderer::DrawBreaker(
    const glm::vec3& Position,
    const glm::vec3& Forward
)
{
    if (!BreakerModel.IsReady())
        return;

    BreakerModel.Draw(
        View,
        Projection,
        CameraPosition,
        Position,
        Forward,
        glm::vec3{1.0f},
        ActiveLightPositions,
        ActiveLightColors,
        ActiveLightCount
    );
}

'''

replace_once(
    "src/Rendering/Renderer.cpp",
    "bool Renderer::HasEntityModels() const\n{",
    breaker_renderer + "bool Renderer::HasEntityModels() const\n{"
)

replace_once(
    "CMakeLists.txt",
    "project(BackroomsOffical VERSION 0.3.22 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.23 LANGUAGES C CXX)"
)

replace_once(
    "src/Core/Version.h",
    'inline constexpr const char* Text = "0.3.22";',
    'inline constexpr const char* Text = "0.3.23";'
)

rc = Path("src/Platform/Windows/Backrooms.rc")
rc_text = rc.read_text().replace("0,3,22,0", "0,3,23,0").replace("0.3.22", "0.3.23")
rc.write_text(rc_text)

Path("update/release_notes.txt").write_text(
    "V0.3.23 replaces the floating placeholder breakers with a real wall-mounted Poly Haven power-box model and proper wall-aligned collision. It upgrades the existing Quaternius entity into a stalking-to-chase encounter with a short warning window, distance-based pursuit speed and positional cues. The native soundscape now uses a recorded CC0 fluorescent-light hum, Kenney CC0 carpet footsteps, a real breaker switch sound and positional metal/entity cues instead of relying on the old synthetic ambience as the primary mix."
)

third_party = Path("THIRD_PARTY_ASSETS.md")
text = third_party.read_text()
addition = r'''

## Breaker model

- `assets/models/power_box_01/power_box_01_1k.gltf`
- `assets/models/power_box_01/power_box_01.bin`
  - Asset: Power Box 01
  - Creators: Rico Cilliers and Yann Kervran
  - Original source: Poly Haven
  - License: CC0 1.0
  - Vendored from the public `lanathlor/pyrrhic-stars` mirror of the Poly Haven model.
  - The native renderer uses the real mesh geometry and a neutral metal base color; external texture files are not required by this build.

## Native environment and interaction audio

- `assets/audio/fluorescent-hum.ogg`
  - Creator: ftpalad
  - Original source: Freesound sound 119910, `Fluorescent Lightbulb Hum.aif`
  - License: CC0 1.0
  - Vendored from a public GitHub copy of the Freesound asset.

- `assets/audio/footstep-carpet-1.ogg` through `footstep-carpet-4.ogg`
  - Creator: Kenney
  - Original pack: Kenney RPG Audio (`cloth1.ogg` through `cloth4.ogg`)
  - License: CC0 1.0
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.

- `assets/audio/breaker-trip.ogg`
  - Creator: Kenney
  - Original pack: Kenney Interface Sounds (`switch_003.ogg`)
  - License: CC0 1.0
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.

- `assets/audio/entity-metal.ogg`
  - Creator: Kenney
  - Original pack: Kenney RPG Audio (`metalPot1.ogg`)
  - License: CC0 1.0
  - Used as a quiet positional distant-metal cue for the entity encounter.
  - Vendored from the public `Sonofg0tham/tailgate` asset mirror and its asset SBOM.
'''

if "## Breaker model" not in text:
    text += addition

text = text.replace(
    "## Procedural audio\n\n- Continuous fluorescent hum is synthesized at runtime with WebAudio oscillators.\n- Continuous static/noise music is synthesized at runtime with a generated noise buffer and filters.\n- Shapeshift static bursts are synthesized at runtime.\n- These procedural audio layers do not contain third-party recordings.\n",
    "## Legacy procedural audio\n\n- Older generated WAV layers remain in the repository as fallback compatibility assets.\n- V0.3.23 and later use the recorded CC0 fluorescent hum and Kenney interaction/footstep recordings above as the primary native soundscape.\n"
)
third_party.write_text(text)
