#pragma once

#include "WorldTypes.h"

#include <array>
#include <cstdint>

class WorldGenerator
{
public:
    explicit WorldGenerator(uint32_t Seed);

    WorldData Build();
    WorldData BuildAround(const glm::vec3& FocusPosition);
    WorldData BuildMapAround(const glm::vec3& FocusPosition);
    bool NeedsRebuild(
        const WorldData& World,
        const glm::vec3& FocusPosition
    ) const;

private:
    static constexpr int ChunkCellCount = 9;
    static constexpr int ChunkHalfCells = ChunkCellCount / 2;
    static constexpr int ActiveChunkRadius = 1;
    static constexpr float DefaultCellSize = 6.0f;

    uint32_t Hash(int X, int Z, uint32_t Salt) const;
    float Random01(int X, int Z, uint32_t Salt) const;
    int ChunkCoordinate(float Coordinate) const;

    void CreateStreamedMaze(WorldData& World);
    void CreateChunkCells(
        int ChunkX,
        int ChunkZ,
        std::array<MazeCell, ChunkCellCount * ChunkCellCount>& Cells
    ) const;

    void RemoveWall(
        MazeCell& Current,
        MazeCell& Next,
        Direction Dir
    ) const;

    void BuildEnvironment(WorldData& World) const;
    void AddWall(
        WorldData& World,
        float X,
        float Z,
        float SizeX,
        float SizeZ
    ) const;

    uint32_t Seed = 1;
};
