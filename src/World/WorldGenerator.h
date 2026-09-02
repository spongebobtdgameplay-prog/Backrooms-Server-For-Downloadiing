#pragma once

#include "WorldTypes.h"

#include <cstdint>

class WorldGenerator
{
public:
    explicit WorldGenerator(uint32_t Seed);

    WorldData Build();

private:
    float Random();
    void CreateMaze(WorldData& World);
    void RemoveWall(WorldData& World, MazeCell& Current, MazeCell& Next, Direction Dir);
    void BuildEnvironment(WorldData& World);
    void AddWall(
        WorldData& World,
        float X,
        float Z,
        float SizeX,
        float SizeZ
    );

    uint32_t RandomState = 1;
};
