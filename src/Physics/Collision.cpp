#include "Collision.h"

#include <algorithm>

namespace Collision
{
    bool CircleIntersectsAABB(
        const glm::vec3& Position,
        float Radius,
        const AABB& Box
    )
    {
        const float ClosestX = std::clamp(Position.x, Box.Min.x, Box.Max.x);
        const float ClosestZ = std::clamp(Position.z, Box.Min.z, Box.Max.z);

        const float DX = Position.x - ClosestX;
        const float DZ = Position.z - ClosestZ;

        return DX * DX + DZ * DZ < Radius * Radius;
    }

    static bool Blocked(
        const glm::vec3& Position,
        float Radius,
        const std::vector<AABB>& Colliders
    )
    {
        for (const AABB& Box : Colliders)
        {
            if (CircleIntersectsAABB(Position, Radius, Box))
                return true;
        }

        return false;
    }

    glm::vec3 ResolveHorizontalMove(
        const glm::vec3& Position,
        const glm::vec3& DesiredMove,
        float Radius,
        const std::vector<AABB>& Colliders
    )
    {
        glm::vec3 Result = Position;

        glm::vec3 TryX = Result;
        TryX.x += DesiredMove.x;

        if (!Blocked(TryX, Radius, Colliders))
            Result.x = TryX.x;

        glm::vec3 TryZ = Result;
        TryZ.z += DesiredMove.z;

        if (!Blocked(TryZ, Radius, Colliders))
            Result.z = TryZ.z;

        return Result;
    }
}
