#pragma once

#include <glm/glm.hpp>

#include <array>
#include <vector>

struct AABB
{
    glm::vec3 Min{0.0f};
    glm::vec3 Max{0.0f};
};

enum class Direction
{
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

enum class SurfaceMaterial
{
    Generic = 0,
    Wallpaper = 1,
    Carpet = 2,
    Ceiling = 3,
    Trim = 4,
    Fixture = 5,
    LightPanel = 6
};

struct MazeCell
{
    int X = 0;
    int Z = 0;
    std::array<bool, 4> Walls{true, true, true, true};
    bool Visited = false;
};

struct SceneBox
{
    glm::vec3 Position{0.0f};
    glm::vec3 Size{1.0f};
    glm::vec3 Color{1.0f};
    glm::vec3 Emissive{0.0f};
    float Roughness = 0.8f;
    int MaterialType = static_cast<int>(SurfaceMaterial::Generic);
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

struct WorldData
{
    int Columns = 27;
    int Rows = 27;
    int ChunkCells = 9;
    int StreamRadius = 1;
    int CenterChunkX = 0;
    int CenterChunkZ = 0;
    int OriginCellX = -13;
    int OriginCellZ = -13;

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

    bool ContainsWorldCell(int WorldX, int WorldZ) const
    {
        const int LocalX = WorldX - OriginCellX;
        const int LocalZ = WorldZ - OriginCellZ;

        return
            LocalX >= 0 &&
            LocalZ >= 0 &&
            LocalX < Columns &&
            LocalZ < Rows;
    }
};
