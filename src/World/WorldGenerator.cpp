#include "WorldGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace
{
    int FloorDivide(int Value, int Divisor)
    {
        int Quotient = Value / Divisor;
        const int Remainder = Value % Divisor;

        if (Remainder < 0)
            --Quotient;

        return Quotient;
    }
}

WorldGenerator::WorldGenerator(uint32_t NewSeed)
{
    Seed = NewSeed == 0 ? 1 : NewSeed;
}

uint32_t WorldGenerator::Hash(
    int X,
    int Z,
    uint32_t Salt
) const
{
    uint32_t Value =
        Seed ^
        static_cast<uint32_t>(X) * 0x9E3779B9u ^
        static_cast<uint32_t>(Z) * 0x85EBCA6Bu ^
        Salt * 0xC2B2AE35u;

    Value ^= Value >> 16;
    Value *= 0x7FEB352Du;
    Value ^= Value >> 15;
    Value *= 0x846CA68Bu;
    Value ^= Value >> 16;

    return Value;
}

float WorldGenerator::Random01(
    int X,
    int Z,
    uint32_t Salt
) const
{
    return
        static_cast<float>(Hash(X, Z, Salt)) /
        4294967296.0f;
}

int WorldGenerator::ChunkCoordinate(float Coordinate) const
{
    const int Cell =
        static_cast<int>(
            std::round(Coordinate / DefaultCellSize)
        );

    return FloorDivide(
        Cell + ChunkHalfCells,
        ChunkCellCount
    );
}

WorldData WorldGenerator::Build()
{
    return BuildAround({0.0f, 0.0f, 0.0f});
}

WorldData WorldGenerator::BuildAround(
    const glm::vec3& FocusPosition
)
{
    WorldData World;

    World.ChunkCells = ChunkCellCount;
    World.StreamRadius = ActiveChunkRadius;
    World.Columns =
        ChunkCellCount * (ActiveChunkRadius * 2 + 1);
    World.Rows = World.Columns;
    World.CellSize = DefaultCellSize;

    World.CenterChunkX = ChunkCoordinate(FocusPosition.x);
    World.CenterChunkZ = ChunkCoordinate(FocusPosition.z);

    const int FirstChunkX =
        World.CenterChunkX - ActiveChunkRadius;

    const int FirstChunkZ =
        World.CenterChunkZ - ActiveChunkRadius;

    World.OriginCellX =
        FirstChunkX * ChunkCellCount -
        ChunkHalfCells;

    World.OriginCellZ =
        FirstChunkZ * ChunkCellCount -
        ChunkHalfCells;

    CreateStreamedMaze(World);
    BuildEnvironment(World);

    return World;
}


WorldData WorldGenerator::BuildMapAround(
    const glm::vec3& FocusPosition
)
{
    return BuildMapRegion(FocusPosition, ActiveChunkRadius);
}

WorldData WorldGenerator::BuildMapRegion(
    const glm::vec3& FocusPosition,
    int ChunkRadius
)
{
    WorldData World;

    const int Radius = std::clamp(ChunkRadius, 1, 40);

    World.ChunkCells = ChunkCellCount;
    World.StreamRadius = Radius;
    World.Columns = ChunkCellCount * (Radius * 2 + 1);
    World.Rows = World.Columns;
    World.CellSize = DefaultCellSize;

    World.CenterChunkX = ChunkCoordinate(FocusPosition.x);
    World.CenterChunkZ = ChunkCoordinate(FocusPosition.z);

    const int FirstChunkX = World.CenterChunkX - Radius;
    const int FirstChunkZ = World.CenterChunkZ - Radius;

    World.OriginCellX =
        FirstChunkX * ChunkCellCount -
        ChunkHalfCells;

    World.OriginCellZ =
        FirstChunkZ * ChunkCellCount -
        ChunkHalfCells;

    CreateStreamedMaze(World);
    return World;
}

bool WorldGenerator::NeedsRebuild(
    const WorldData& World,
    const glm::vec3& FocusPosition
) const
{
    return
        ChunkCoordinate(FocusPosition.x) != World.CenterChunkX ||
        ChunkCoordinate(FocusPosition.z) != World.CenterChunkZ;
}

void WorldGenerator::RemoveWall(
    MazeCell& Current,
    MazeCell& Next,
    Direction Dir
) const
{
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

void WorldGenerator::CreateChunkCells(
    int ChunkX,
    int ChunkZ,
    std::array<MazeCell, ChunkCellCount * ChunkCellCount>& Cells
) const
{
    auto CellAt = [&](int X, int Z) -> MazeCell&
    {
        return Cells[static_cast<std::size_t>(Z * ChunkCellCount + X)];
    };

    for (int Z = 0; Z < ChunkCellCount; ++Z)
    {
        for (int X = 0; X < ChunkCellCount; ++X)
        {
            MazeCell& Cell = CellAt(X, Z);
            Cell.X = X;
            Cell.Z = Z;
            Cell.Visited = false;
            Cell.Walls = {true, true, true, true};
        }
    }

    uint32_t State = Hash(ChunkX, ChunkZ, 0x1347u);

    auto NextRandom = [&]()
    {
        State = 1664525u * State + 1013904223u;
        return State;
    };

    const int StartIndex =
        static_cast<int>(NextRandom() % Cells.size());

    MazeCell* Current =
        &Cells[static_cast<std::size_t>(StartIndex)];

    Current->Visited = true;

    std::vector<MazeCell*> Stack;
    Stack.reserve(Cells.size());

    int Visited = 1;

    while (Visited < static_cast<int>(Cells.size()))
    {
        std::array<std::pair<Direction, MazeCell*>, 4> Candidates{};
        int CandidateCount = 0;

        const int X = Current->X;
        const int Z = Current->Z;

        if (Z > 0 && !CellAt(X, Z - 1).Visited)
            Candidates[static_cast<std::size_t>(CandidateCount++)] =
                {Direction::North, &CellAt(X, Z - 1)};

        if (
            X < ChunkCellCount - 1 &&
            !CellAt(X + 1, Z).Visited
        )
        {
            Candidates[static_cast<std::size_t>(CandidateCount++)] =
                {Direction::East, &CellAt(X + 1, Z)};
        }

        if (
            Z < ChunkCellCount - 1 &&
            !CellAt(X, Z + 1).Visited
        )
        {
            Candidates[static_cast<std::size_t>(CandidateCount++)] =
                {Direction::South, &CellAt(X, Z + 1)};
        }

        if (X > 0 && !CellAt(X - 1, Z).Visited)
            Candidates[static_cast<std::size_t>(CandidateCount++)] =
                {Direction::West, &CellAt(X - 1, Z)};

        if (CandidateCount > 0)
        {
            const int Choice =
                static_cast<int>(
                    NextRandom() %
                    static_cast<uint32_t>(CandidateCount)
                );

            const auto [Dir, Next] =
                Candidates[static_cast<std::size_t>(Choice)];

            RemoveWall(*Current, *Next, Dir);
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

    for (int I = 0; I < 22; ++I)
    {
        const int X =
            static_cast<int>(NextRandom() % ChunkCellCount);

        const int Z =
            static_cast<int>(NextRandom() % ChunkCellCount);

        const Direction Dir =
            static_cast<Direction>(NextRandom() % 4u);

        MazeCell& Cell = CellAt(X, Z);

        if (Dir == Direction::North && Z > 0)
            RemoveWall(Cell, CellAt(X, Z - 1), Dir);

        if (Dir == Direction::East && X < ChunkCellCount - 1)
            RemoveWall(Cell, CellAt(X + 1, Z), Dir);

        if (Dir == Direction::South && Z < ChunkCellCount - 1)
            RemoveWall(Cell, CellAt(X, Z + 1), Dir);

        if (Dir == Direction::West && X > 0)
            RemoveWall(Cell, CellAt(X - 1, Z), Dir);
    }

    CellAt(ChunkHalfCells, 0).Walls[0] = false;
    CellAt(ChunkCellCount - 1, ChunkHalfCells).Walls[1] = false;
    CellAt(ChunkHalfCells, ChunkCellCount - 1).Walls[2] = false;
    CellAt(0, ChunkHalfCells).Walls[3] = false;
}

void WorldGenerator::CreateStreamedMaze(WorldData& World)
{
    World.Cells.resize(
        static_cast<std::size_t>(World.Columns * World.Rows)
    );

    const int FirstChunkX =
        World.CenterChunkX - World.StreamRadius;

    const int FirstChunkZ =
        World.CenterChunkZ - World.StreamRadius;

    for (
        int ChunkZ = FirstChunkZ;
        ChunkZ <= World.CenterChunkZ + World.StreamRadius;
        ++ChunkZ
    )
    {
        for (
            int ChunkX = FirstChunkX;
            ChunkX <= World.CenterChunkX + World.StreamRadius;
            ++ChunkX
        )
        {
            std::array<
                MazeCell,
                ChunkCellCount * ChunkCellCount
            > ChunkCells{};

            CreateChunkCells(
                ChunkX,
                ChunkZ,
                ChunkCells
            );

            const int ChunkWorldCellX =
                ChunkX * ChunkCellCount -
                ChunkHalfCells;

            const int ChunkWorldCellZ =
                ChunkZ * ChunkCellCount -
                ChunkHalfCells;

            const int BaseLocalX =
                (ChunkX - FirstChunkX) *
                ChunkCellCount;

            const int BaseLocalZ =
                (ChunkZ - FirstChunkZ) *
                ChunkCellCount;

            for (int LocalZ = 0; LocalZ < ChunkCellCount; ++LocalZ)
            {
                for (int LocalX = 0; LocalX < ChunkCellCount; ++LocalX)
                {
                    const MazeCell& Source =
                        ChunkCells[static_cast<std::size_t>(
                            LocalZ * ChunkCellCount + LocalX
                        )];

                    MazeCell& Target = World.Cell(
                        BaseLocalX + LocalX,
                        BaseLocalZ + LocalZ
                    );

                    Target = Source;
                    Target.X = ChunkWorldCellX + LocalX;
                    Target.Z = ChunkWorldCellZ + LocalZ;
                    Target.Visited = true;
                }
            }
        }
    }
}

void WorldGenerator::AddWall(
    WorldData& World,
    float X,
    float Z,
    float SizeX,
    float SizeZ
) const
{
    const glm::vec3 Position{
        X,
        World.WallHeight * 0.5f,
        Z
    };

    const glm::vec3 Size{
        SizeX,
        World.WallHeight,
        SizeZ
    };

    World.Boxes.push_back({
        Position,
        Size,
        {0.5984f, 0.4937f, 0.1029f},
        {0.01227f, 0.01090f, 0.00443f},
        0.99f,
        static_cast<int>(SurfaceMaterial::Wallpaper)
    });

    World.Colliders.push_back({
        {
            X - SizeX * 0.5f,
            0.0f,
            Z - SizeZ * 0.5f
        },
        {
            X + SizeX * 0.5f,
            World.WallHeight,
            Z + SizeZ * 0.5f
        }
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

void WorldGenerator::BuildEnvironment(WorldData& World) const
{
    const float Width =
        static_cast<float>(World.Columns) *
        World.CellSize;

    const float Depth =
        static_cast<float>(World.Rows) *
        World.CellSize;

    const float CenterX =
        (
            static_cast<float>(World.OriginCellX) +
            static_cast<float>(World.Columns - 1) * 0.5f
        ) *
        World.CellSize;

    const float CenterZ =
        (
            static_cast<float>(World.OriginCellZ) +
            static_cast<float>(World.Rows - 1) * 0.5f
        ) *
        World.CellSize;

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

    World.OpenCells.reserve(
        static_cast<std::size_t>(World.Columns * World.Rows)
    );

    for (int LocalZ = 0; LocalZ < World.Rows; ++LocalZ)
    {
        for (int LocalX = 0; LocalX < World.Columns; ++LocalX)
        {
            const MazeCell& Cell =
                World.Cell(LocalX, LocalZ);

            const float CellX =
                static_cast<float>(Cell.X) *
                World.CellSize;

            const float CellZ =
                static_cast<float>(Cell.Z) *
                World.CellSize;

            World.OpenCells.push_back({
                CellX,
                0.0f,
                CellZ
            });

            if (Cell.Walls[0])
            {
                AddWall(
                    World,
                    CellX,
                    CellZ - World.CellSize * 0.5f,
                    World.CellSize + World.WallThickness,
                    World.WallThickness
                );
            }

            if (Cell.Walls[3])
            {
                AddWall(
                    World,
                    CellX - World.CellSize * 0.5f,
                    CellZ,
                    World.WallThickness,
                    World.CellSize + World.WallThickness
                );
            }

            if (
                LocalZ == World.Rows - 1 &&
                Cell.Walls[2]
            )
            {
                AddWall(
                    World,
                    CellX,
                    CellZ + World.CellSize * 0.5f,
                    World.CellSize + World.WallThickness,
                    World.WallThickness
                );
            }

            if (
                LocalX == World.Columns - 1 &&
                Cell.Walls[1]
            )
            {
                AddWall(
                    World,
                    CellX + World.CellSize * 0.5f,
                    CellZ,
                    World.WallThickness,
                    World.CellSize + World.WallThickness
                );
            }

            if (Random01(Cell.X, Cell.Z, 1u) > 0.46f)
            {
                const float OffsetX =
                    (Random01(Cell.X, Cell.Z, 2u) - 0.5f) *
                    2.15f;

                const float OffsetZ =
                    (Random01(Cell.X, Cell.Z, 3u) - 0.5f) *
                    2.15f;

                LightPoint Light;

                Light.Position = {
                    CellX + OffsetX,
                    World.WallHeight - 0.18f,
                    CellZ + OffsetZ
                };

                Light.Color = {
                    1.0f,
                    0.8796f,
                    0.5520f
                };

                Light.BaseIntensity =
                    1.25f +
                    Random01(Cell.X, Cell.Z, 4u) * 0.65f;

                Light.FlickerSpeed =
                    20.0f +
                    Random01(Cell.X, Cell.Z, 5u) * 22.0f;

                Light.FlickerStrength =
                    0.015f +
                    Random01(Cell.X, Cell.Z, 6u) * 0.055f;

                Light.Phase =
                    Random01(Cell.X, Cell.Z, 7u) * 100.0f;

                Light.Faulty =
                    Random01(Cell.X, Cell.Z, 8u) < 0.14f;

                World.Lights.push_back(Light);

                const bool Rotate =
                    Random01(Cell.X, Cell.Z, 9u) > 0.5f;

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

            if (
                LocalX <= 0 ||
                LocalZ <= 0 ||
                LocalX >= World.Columns - 1 ||
                LocalZ >= World.Rows - 1 ||
                Random01(Cell.X, Cell.Z, 10u) > 0.075f
            )
            {
                continue;
            }

            const float ColumnX =
                CellX +
                (
                    Random01(Cell.X, Cell.Z, 11u) > 0.5f
                        ? 1.7f
                        : -1.7f
                );

            const float ColumnZ =
                CellZ +
                (
                    Random01(Cell.X, Cell.Z, 12u) > 0.5f
                        ? 1.7f
                        : -1.7f
                );

            World.Boxes.push_back({
                {
                    ColumnX,
                    World.WallHeight * 0.5f,
                    ColumnZ
                },
                {
                    0.52f,
                    World.WallHeight,
                    0.52f
                },
                {0.5984f, 0.4937f, 0.1029f},
                {0.01227f, 0.01090f, 0.00443f},
                0.99f,
                static_cast<int>(SurfaceMaterial::Wallpaper)
            });

            World.Colliders.push_back({
                {
                    ColumnX - 0.26f,
                    0.0f,
                    ColumnZ - 0.26f
                },
                {
                    ColumnX + 0.26f,
                    World.WallHeight,
                    ColumnZ + 0.26f
                }
            });
        }
    }
}
