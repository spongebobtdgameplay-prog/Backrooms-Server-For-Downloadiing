#pragma once

#include "../World/WorldTypes.h"

#include <glm/glm.hpp>

#include <vector>

struct Ray
{
    glm::vec3 Origin{0.0f};
    glm::vec3 Direction{0.0f, 0.0f, -1.0f};
};

struct RayHit
{
    bool Hit = false;
    float Distance = 0.0f;
    glm::vec3 Point{0.0f};
    glm::vec3 Normal{0.0f};
    int Index = -1;
};

namespace Raycast
{
    RayHit AgainstAABB(
        const Ray& RayData,
        const AABB& Box,
        float MaxDistance
    );

    RayHit AgainstWorld(
        const Ray& RayData,
        const std::vector<AABB>& Boxes,
        float MaxDistance
    );
}
