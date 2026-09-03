#include "WorldGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

WorldGenerator::WorldGenerator(uint32_t Seed)
{
    RandomState = Seed == 0 ? 1 : Seed;
}

float WorldGenerator::Random()
{
    RandomState = 1664525u * RandomState + 1013904223u;
    return static_cast<float>(RandomState) / 4294967296.0f;
}

WorldData WorldGenerator::Build()
{
    WorldData World;
    CreateMaze(World);
    BuildEnvironment(World);
    return World;
}

void WorldGenerator::CreateMaze(WorldData& World)
{
    World.Cells.resize(static_cast<std::size_t>(World.Columns * World.Rows));

    for (int Z = 0; Z < World.Rows; ++Z)
    {
        for (int X = 0; X < World.Columns; ++X)
        {
            MazeCell& Cell = World.Cell(X, Z);
            Cell.X = X;
            Cell.Z = Z;
            Cell.Visited = false;
            Cell.Walls = {true, true, true, true};
        }
    }

    std::vector<MazeCell*> Stack;
    MazeCell* Current = &World.Cell(0, 0);
    Current->Visited = true;

    int Visited = 1;
    const int Total = World.Columns * World.Rows;

    while (Visited < Total)
    {
        std::vector<std::pair<Direction, MazeCell*>> Neighbors;
        const int X = Current->X;
        const int Z = Current->Z;

        if (Z > 0 && !World.Cell(X, Z - 1).Visited)
            Neighbors.emplace_back(Direction::North, &World.Cell(X, Z - 1));

        if (X < World.Columns - 1 && !World.Cell(X + 1, Z).Visited)
            Neighbors.emplace_back(Direction::East, &World.Cell(X + 1, Z));

        if (Z < World.Rows - 1 && !World.Cell(X, Z + 1).Visited)
            Neighbors.emplace_back(Direction::South, &World.Cell(X, Z + 1));

        if (X > 0 && !World.Cell(X - 1, Z).Visited)
            Neighbors.emplace_back(Direction::West, &World.Cell(X - 1, Z));

        if (!Neighbors.empty())
        {
            const std::size_t Index = static_cast<std::size_t>(
                Random() * static_cast<float>(Neighbors.size())
            ) % Neighbors.size();

            auto [Dir, Next] = Neighbors[Index];

            RemoveWall(World, *Current, *Next, Dir);
            Stack.push_back(Current);

            Current = Next;
            Current->Visited = true;
            ++Visited;
        }
        else
        {
            Current = Stack.back();
            Stack.pop_back();
        }
    }

    for (int I = 0; I < 68; ++I)
    {
        const int X = 1 + static_cast<int>(Random() * static_cast<float>(World.Columns - 2));
        const int Z = 1 + static_cast<int>(Random() * static_cast<float>(World.Rows - 2));

        MazeCell& Cell = World.Cell(X, Z);
        const Direction Dir = static_cast<Direction>(static_cast<int>(Random() * 4.0f) % 4);

        if (Dir == Direction::North)
            RemoveWall(World, Cell, World.Cell(X, Z - 1), Dir);

        if (Dir == Direction::East)
            RemoveWall(World, Cell, World.Cell(X + 1, Z), Dir);

        if (Dir == Direction::South)
            RemoveWall(World, Cell, World.Cell(X, Z + 1), Dir);

        if (Dir == Direction::West)
            RemoveWall(World, Cell, World.Cell(X - 1, Z), Dir);
    }
}

void WorldGenerator::RemoveWall(
    WorldData& World,
    MazeCell& Current,
    MazeCell& Next,
    Direction Dir
)
{
    static_cast<void>(World);

    if (Dir == Direction::North)
    {
        Current.Walls[0] = false;
        Next.Walls[2] = false;
    }

    if (Dir == Direction::East)
    {
        Current.Walls[1] = false;
        Next.Walls[3] = false;
    }

    if (Dir == Direction::South)
    {
        Current.Walls[2] = false;
        Next.Walls[0] = false;
    }

    if (Dir == Direction::West)
    {
        Current.Walls[3] = false;
        Next.Walls[1] = false;
    }
}

void WorldGenerator::AddWall(
    WorldData& World,
    float X,
    float Z,
    float SizeX,
    float SizeZ
)
{
    const glm::vec3 Position{X, World.WallHeight * 0.5f, Z};
    const glm::vec3 Size{SizeX, World.WallHeight, SizeZ};

    World.Boxes.push_back({
        Position,
        Size,
        {0.86f, 0.82f, 0.53f},
        {0.045f, 0.042f, 0.018f},
        0.99f,
        static_cast<int>(SurfaceMaterial::Wallpaper)
    });

    World.Colliders.push_back({
        {X - SizeX * 0.5f, 0.0f, Z - SizeZ * 0.5f},
        {X + SizeX * 0.5f, World.WallHeight, Z + SizeZ * 0.5f}
    });

    const glm::vec3 TrimSize{
        SizeX + (SizeX > SizeZ ? 0.04f : 0.035f),
        0.13f,
        SizeZ + (SizeZ > SizeX ? 0.04f : 0.035f)
    };

    World.Boxes.push_back({
        {X, 0.065f, Z},
        TrimSize,
        {0.69f, 0.66f, 0.53f},
        {0.015f, 0.014f, 0.01f},
        1.0f,
        static_cast<int>(SurfaceMaterial::Trim)
    });
}

void WorldGenerator::BuildEnvironment(WorldData& World)
{
    const float Width = static_cast<float>(World.Columns) * World.CellSize;
    const float Depth = static_cast<float>(World.Rows) * World.CellSize;
    const float CenterX = Width * 0.5f - World.CellSize * 0.5f;
    const float CenterZ = Depth * 0.5f - World.CellSize * 0.5f;

    World.Boxes.push_back({
        {CenterX, -0.06f, CenterZ},
        {Width, 0.12f, Depth},
        {0.47f, 0.45f, 0.38f},
        {0.008f, 0.008f, 0.006f},
        1.0f,
        static_cast<int>(SurfaceMaterial::Carpet)
    });

    World.Boxes.push_back({
        {CenterX, World.WallHeight + 0.06f, CenterZ},
        {Width, 0.12f, Depth},
        {0.78f, 0.76f, 0.65f},
        {0.03f, 0.029f, 0.022f},
        0.97f,
        static_cast<int>(SurfaceMaterial::Ceiling)
    });

    for (int Z = 0; Z < World.Rows; ++Z)
    {
        for (int X = 0; X < World.Columns; ++X)
        {
            const MazeCell& Cell = World.Cell(X, Z);
            const float CellX = static_cast<float>(X) * World.CellSize;
            const float CellZ = static_cast<float>(Z) * World.CellSize;

            World.OpenCells.push_back({CellX, 0.0f, CellZ});

            if (Cell.Walls[0])
                AddWall(
                    World,
                    CellX,
                    CellZ - World.CellSize * 0.5f,
                    World.CellSize + World.WallThickness,
                    World.WallThickness
                );

            if (Cell.Walls[3])
                AddWall(
                    World,
                    CellX - World.CellSize * 0.5f,
                    CellZ,
                    World.WallThickness,
                    World.CellSize + World.WallThickness
                );

            if (Z == World.Rows - 1 && Cell.Walls[2])
                AddWall(
                    World,
                    CellX,
                    CellZ + World.CellSize * 0.5f,
                    World.CellSize + World.WallThickness,
                    World.WallThickness
                );

            if (X == World.Columns - 1 && Cell.Walls[1])
                AddWall(
                    World,
                    CellX + World.CellSize * 0.5f,
                    CellZ,
                    World.WallThickness,
                    World.CellSize + World.WallThickness
                );

            if (Random() > 0.46f)
            {
                const float OffsetX = (Random() - 0.5f) * 2.15f;
                const float OffsetZ = (Random() - 0.5f) * 2.15f;

                LightPoint Light;
                Light.Position = {
                    CellX + OffsetX,
                    World.WallHeight - 0.18f,
                    CellZ + OffsetZ
                };
                Light.Color = {1.0f, 0.93f, 0.72f};
                Light.BaseIntensity = 1.25f + Random() * 0.65f;
                Light.FlickerSpeed = 20.0f + Random() * 22.0f;
                Light.FlickerStrength = 0.015f + Random() * 0.055f;
                Light.Phase = Random() * 100.0f;
                Light.Faulty = Random() < 0.14f;

                World.Lights.push_back(Light);

                const bool Rotate = Random() > 0.5f;

                World.Boxes.push_back({
                    {
                        Light.Position.x,
                        World.WallHeight - 0.055f,
                        Light.Position.z
                    },
                    Rotate
                        ? glm::vec3{0.52f, 0.04f, 1.42f}
                        : glm::vec3{1.42f, 0.04f, 0.52f},
                    {0.66f, 0.63f, 0.52f},
                    {0.025f, 0.024f, 0.018f},
                    0.95f,
                    static_cast<int>(SurfaceMaterial::Fixture)
                });

                World.Boxes.push_back({
                    {
                        Light.Position.x,
                        World.WallHeight - 0.082f,
                        Light.Position.z
                    },
                    Rotate
                        ? glm::vec3{0.34f, 0.022f, 1.19f}
                        : glm::vec3{1.19f, 0.022f, 0.34f},
                    {1.0f, 0.94f, 0.74f},
                    {0.52f, 0.47f, 0.31f},
                    0.8f,
                    static_cast<int>(SurfaceMaterial::LightPanel)
                });
            }
        }
    }

    for (int Z = 1; Z < World.Rows - 1; ++Z)
    {
        for (int X = 1; X < World.Columns - 1; ++X)
        {
            if (Random() > 0.075f)
                continue;

            const float ColumnX =
                static_cast<float>(X) * World.CellSize +
                (Random() > 0.5f ? 1.7f : -1.7f);

            const float ColumnZ =
                static_cast<float>(Z) * World.CellSize +
                (Random() > 0.5f ? 1.7f : -1.7f);

            World.Boxes.push_back({
                {ColumnX, World.WallHeight * 0.5f, ColumnZ},
                {0.52f, World.WallHeight, 0.52f},
                {0.86f, 0.82f, 0.53f},
                {0.04f, 0.037f, 0.018f},
                0.99f,
                static_cast<int>(SurfaceMaterial::Wallpaper)
            });

            World.Colliders.push_back({
                {ColumnX - 0.26f, 0.0f, ColumnZ - 0.26f},
                {ColumnX + 0.26f, World.WallHeight, ColumnZ + 0.26f}
            });
        }
    }
}
