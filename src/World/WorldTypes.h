#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <vector>

struct AABB
{
    glm::vec3 Min{0.0f};
    glm::vec3 Max{0.0f};
};

struct SceneBox
{
    glm::vec3 Position{0.0f};
    glm::vec3 Size{1.0f};
    glm::vec3 Color{1.0f};
    glm::vec3 Emissive{0.0f};
    float Roughness = 1.0f;
};

struct LightPoint
{
    glm::vec3 Position{0.0f};
    glm::vec3 Color{1.0f};
    float BaseIntensity = 1.0f;
    float FlickerSpeed = 20.0f;
    float FlickerStrength = 0.03f;
    float Phase = 0.0f;
    bool Faulty = false;
};

struct MazeCell
{
    int X = 0;
    int Z = 0;
    bool Visited = false;
    std::array<bool, 4> Walls{true, true, true, true};
};

enum class Direction : int
{
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

struct WorldData
{
    int Columns = 15;
    int Rows = 15;
    float CellSize = 6.0f;
    float WallHeight = 3.18f;
    float WallThickness = 0.24f;

    std::vector<MazeCell> Cells;
    std::vector<AABB> Colliders;
    std::vector<SceneBox> Boxes;
    std::vector<glm::vec3> OpenCells;
    std::vector<LightPoint> Lights;

    MazeCell& Cell(int X, int Z)
    {
        return Cells[static_cast<std::size_t>(Z * Columns + X)];
    }

    const MazeCell& Cell(int X, int Z) const
    {
        return Cells[static_cast<std::size_t>(Z * Columns + X)];
    }
};
