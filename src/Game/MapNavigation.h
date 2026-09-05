#pragma once

#include "../World/WorldTypes.h"

#include <glm/glm.hpp>

#include <vector>

namespace MapNavigation
{
    std::vector<glm::vec2> FindPath(
        const WorldData& World,
        const glm::vec2& Start,
        const glm::vec2& Goal
    );
}
