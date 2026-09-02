#include "Raycast.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Raycast
{
    RayHit AgainstAABB(
        const Ray& RayData,
        const AABB& Box,
        float MaxDistance
    )
    {
        float TMin = 0.0f;
        float TMax = MaxDistance;
        glm::vec3 HitNormal{0.0f};

        for (int Axis = 0; Axis < 3; ++Axis)
        {
            const float Origin = RayData.Origin[Axis];
            const float Direction = RayData.Direction[Axis];
            const float Min = Box.Min[Axis];
            const float Max = Box.Max[Axis];

            if (std::abs(Direction) < 0.000001f)
            {
                if (Origin < Min || Origin > Max)
                    return {};

                continue;
            }

            float T1 = (Min - Origin) / Direction;
            float T2 = (Max - Origin) / Direction;
            float Sign = -1.0f;

            if (T1 > T2)
            {
                std::swap(T1, T2);
                Sign = 1.0f;
            }

            if (T1 > TMin)
            {
                TMin = T1;
                HitNormal = {0.0f, 0.0f, 0.0f};
                HitNormal[Axis] = Sign;
            }

            TMax = std::min(TMax, T2);

            if (TMin > TMax)
                return {};
        }

        if (TMin < 0.0f || TMin > MaxDistance)
            return {};

        RayHit Result;
        Result.Hit = true;
        Result.Distance = TMin;
        Result.Point = RayData.Origin + RayData.Direction * TMin;
        Result.Normal = HitNormal;

        return Result;
    }

    RayHit AgainstWorld(
        const Ray& RayData,
        const std::vector<AABB>& Boxes,
        float MaxDistance
    )
    {
        RayHit Best;
        float BestDistance = MaxDistance;

        for (std::size_t I = 0; I < Boxes.size(); ++I)
        {
            RayHit Hit = AgainstAABB(RayData, Boxes[I], BestDistance);

            if (!Hit.Hit)
                continue;

            Best = Hit;
            Best.Index = static_cast<int>(I);
            BestDistance = Hit.Distance;
        }

        return Best;
    }
}
