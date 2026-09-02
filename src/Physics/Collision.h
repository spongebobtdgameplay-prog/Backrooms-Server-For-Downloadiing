#pragma once

#include "../World/WorldTypes.h"

#include <glm/glm.hpp>

#include <vector>

namespace Collision
{
    bool CircleIntersectsAABB(
        const glm::vec3& Position,
        float Radius,
        const AABB& Box
    );

    glm::vec3 ResolveHorizontalMove(
        const glm::vec3& Position,
        const glm::vec3& DesiredMove,
        float Radius,
        const std::vector<AABB>& Colliders
    );
}
