#pragma once

#include "../World/WorldTypes.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

class Entity
{
public:
    void Reset(const glm::vec3& StartPosition, uint32_t WorldSeed);
    void Release();

    bool Update(
        float DeltaTime,
        const glm::vec3& PlayerPosition,
        const WorldData& World
    );

    float DistanceTo(const glm::vec3& Point) const;
    std::vector<SceneBox> BuildRenderBoxes() const;

    bool IsActive() const { return Active; }
    bool IsDemonForm() const { return DemonForm; }
    bool PreviousWasDemonForm() const { return PreviousDemonForm; }
    float ShiftAmount() const { return ShiftProgress; }
    const glm::vec3& Position() const { return EntityPosition; }
    const glm::vec3& Forward() const { return Direction; }

    bool ConsumeShifted()
    {
        const bool Result = ShiftedThisFrame;
        ShiftedThisFrame = false;
        return Result;
    }

private:
    int CellIndex(
        const glm::vec3& Position,
        const WorldData& World
    ) const;

    std::vector<int> Neighbors(
        int CellIndex,
        const WorldData& World
    ) const;

    void BuildPath(
        const glm::vec3& Target,
        const WorldData& CollisionWorld
    );

    glm::vec3 EntityPosition{0.0f};
    glm::vec3 Direction{0.0f, 0.0f, -1.0f};

    uint32_t Seed = 1;
    float Radius = 0.28f;
    float RepathTimer = 0.0f;
    float RepathInterval = 0.11f;
    float StuckTimer = 0.0f;

    float ShiftTimer = 7.0f;
    float ShiftProgress = 1.0f;
    float ReleaseGraceTimer = 0.0f;
    float EncounterAge = 0.0f;

    bool Active = false;
    bool DemonForm = false;
    bool PreviousDemonForm = false;
    bool ShiftedThisFrame = false;

    int TargetWorldX = 0;
    int TargetWorldZ = 0;
    std::vector<glm::vec3> Path;
    std::size_t PathIndex = 0;
};
