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
        {0.5984f, 0.4937f, 0.1029f},
        {0.01227f, 0.01090f, 0.00443f},
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
        {0.6240f, 0.5711f, 0.3864f},
        {0.00327f, 0.00306f, 0.00193f},
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
        {0.1904f, 0.1611f, 0.0884f},
        {0.00412f, 0.00397f, 0.00290f},
        1.0f,
        static_cast<int>(SurfaceMaterial::Carpet)
    });

    World.Boxes.push_back({
        {CenterX, World.WallHeight + 0.06f, CenterZ},
        {Width, 0.12f, Depth},
        {0.5595f, 0.5004f, 0.2800f},
        {0.01831f, 0.01674f, 0.00952f},
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
                Light.Color = {1.0f, 0.8796f, 0.5520f};
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
                    {0.4793f, 0.4342f, 0.3185f},
                    {0.00206f, 0.00191f, 0.00127f},
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
                    {1.0f, 0.9047f, 0.6240f},
                    {0.0f, 0.0f, 0.0f},
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
                {0.5984f, 0.4937f, 0.1029f},
                {0.01227f, 0.01090f, 0.00443f},
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
