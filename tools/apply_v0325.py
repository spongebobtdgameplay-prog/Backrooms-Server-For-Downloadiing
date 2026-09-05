from pathlib import Path
import re

Root = Path('.')

def Read(PathName):
    return (Root / PathName).read_text(encoding='utf-8')

def Write(PathName, Content):
    PathObject = Root / PathName
    PathObject.parent.mkdir(parents=True, exist_ok=True)
    PathObject.write_text(Content, encoding='utf-8', newline='\n')

def Replace(PathName, Old, New, Count=1):
    Content = Read(PathName)
    Found = Content.count(Old)
    if Found < Count:
        raise RuntimeError(f'{PathName}: expected at least {Count} occurrence(s), found {Found}')
    Content = Content.replace(Old, New, Count)
    Write(PathName, Content)

WorldTypes = r'''#pragma once

#include "../Physics/AABB.h"

#include <glm/glm.hpp>

#include <array>
#include <vector>

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
'''

WorldGeneratorH = r'''#pragma once

#include "WorldTypes.h"

#include <array>
#include <cstdint>

class WorldGenerator
{
public:
    explicit WorldGenerator(uint32_t Seed);

    WorldData Build();
    WorldData BuildAround(const glm::vec3& FocusPosition);
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
'''

WorldGeneratorCpp = r'''#include "WorldGenerator.h"

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
'''

Write('src/World/WorldTypes.h', WorldTypes)
Write('src/World/WorldGenerator.h', WorldGeneratorH)
Write('src/World/WorldGenerator.cpp', WorldGeneratorCpp)

Replace(
    'src/Entity/Entity.cpp',
    '''    const int X = std::clamp(\n        static_cast<int>(std::round(Position.x / World.CellSize)),\n        0,\n        World.Columns - 1\n    );\n\n    const int Z = std::clamp(\n        static_cast<int>(std::round(Position.z / World.CellSize)),\n        0,\n        World.Rows - 1\n    );\n\n    return Z * World.Columns + X;''',
    '''    const int WorldX =\n        static_cast<int>(\n            std::round(Position.x / World.CellSize)\n        );\n\n    const int WorldZ =\n        static_cast<int>(\n            std::round(Position.z / World.CellSize)\n        );\n\n    const int LocalX = std::clamp(\n        WorldX - World.OriginCellX,\n        0,\n        World.Columns - 1\n    );\n\n    const int LocalZ = std::clamp(\n        WorldZ - World.OriginCellZ,\n        0,\n        World.Rows - 1\n    );\n\n    return LocalZ * World.Columns + LocalX;'''
)

OldNeighbors = '''    const MazeCell& Cell =\n        World.Cells[static_cast<std::size_t>(Index)];\n\n    std::vector<int> Result;\n\n    if (!Cell.Walls[0] && Cell.Z > 0)\n        Result.push_back((Cell.Z - 1) * World.Columns + Cell.X);\n\n    if (!Cell.Walls[1] && Cell.X < World.Columns - 1)\n        Result.push_back(Cell.Z * World.Columns + Cell.X + 1);\n\n    if (!Cell.Walls[2] && Cell.Z < World.Rows - 1)\n        Result.push_back((Cell.Z + 1) * World.Columns + Cell.X);\n\n    if (!Cell.Walls[3] && Cell.X > 0)\n        Result.push_back(Cell.Z * World.Columns + Cell.X - 1);\n\n    return Result;'''

NewNeighbors = '''    const MazeCell& Cell =\n        World.Cells[static_cast<std::size_t>(Index)];\n\n    const int LocalX = Index % World.Columns;\n    const int LocalZ = Index / World.Columns;\n\n    std::vector<int> Result;\n\n    if (!Cell.Walls[0] && LocalZ > 0)\n        Result.push_back(Index - World.Columns);\n\n    if (!Cell.Walls[1] && LocalX < World.Columns - 1)\n        Result.push_back(Index + 1);\n\n    if (!Cell.Walls[2] && LocalZ < World.Rows - 1)\n        Result.push_back(Index + World.Columns);\n\n    if (!Cell.Walls[3] && LocalX > 0)\n        Result.push_back(Index - 1);\n\n    return Result;'''

Replace('src/Entity/Entity.cpp', OldNeighbors, NewNeighbors)

Replace(
    'src/Game/Game.cpp',
    '''    const int X = std::clamp(\n        static_cast<int>(std::round(CellCenter.x / World.CellSize)),\n        0,\n        World.Columns - 1\n    );\n\n    const int Z = std::clamp(\n        static_cast<int>(std::round(CellCenter.z / World.CellSize)),\n        0,\n        World.Rows - 1\n    );\n\n    const MazeCell& Cell = World.Cell(X, Z);''',
    '''    const int WorldX =\n        static_cast<int>(\n            std::round(CellCenter.x / World.CellSize)\n        );\n\n    const int WorldZ =\n        static_cast<int>(\n            std::round(CellCenter.z / World.CellSize)\n        );\n\n    const int X = WorldX - World.OriginCellX;\n    const int Z = WorldZ - World.OriginCellZ;\n\n    if (\n        X < 0 ||\n        Z < 0 ||\n        X >= World.Columns ||\n        Z >= World.Rows\n    )\n    {\n        return false;\n    }\n\n    const MazeCell& Cell = World.Cell(X, Z);'''
)

Replace(
    'src/Game/Game.cpp',
    '''            static_cast<uint32_t>((X + 1) * 3266489917u) ^\n            static_cast<uint32_t>((Z + 1) * 668265263u);''',
    '''            static_cast<uint32_t>((WorldX + 4097) * 3266489917u) ^\n            static_cast<uint32_t>((WorldZ + 8191) * 668265263u);'''
)

Replace(
    'src/Game/Game.cpp',
    '    World = Generator.Build();',
    '    World = Generator.BuildAround({0.0f, 0.0f, 0.0f});'
)

Replace(
    'src/Game/Game.cpp',
    '''    const glm::vec3 EntityPosition = Pick(50.0f);''',
    '''    World.Colliders.push_back(ExitBounds());\n\n    const glm::vec3 EntityPosition = Pick(50.0f);'''
)

Replace(
    'src/Game/Game.cpp',
    '''            Hunter.Release();\n            Message = "SOMETHING HEARD THAT";''',
    '''            Hunter.Release();\n            Audio.PlayEntityRelease(Hunter.Position());\n            Message = "SOMETHING HEARD THAT";'''
)

Replace(
    'src/Game/Game.cpp',
    '''    GamePlayer.Update(\n        DeltaTime,\n        World.Colliders,\n        MouseCaptured\n    );\n\n    glm::vec3 PlayerTravel =''',
    '''    GamePlayer.Update(\n        DeltaTime,\n        World.Colliders,\n        MouseCaptured\n    );\n\n    WorldGenerator Streamer(Seed);\n\n    if (Streamer.NeedsRebuild(World, GamePlayer.Position()))\n    {\n        WorldData StreamedWorld =\n            Streamer.BuildAround(GamePlayer.Position());\n\n        for (const Breaker& BreakerData : Breakers)\n        {\n            StreamedWorld.Colliders.push_back(\n                BreakerBounds(BreakerData)\n            );\n        }\n\n        StreamedWorld.Colliders.push_back(ExitBounds());\n        World = std::move(StreamedWorld);\n    }\n\n    glm::vec3 PlayerTravel ='''
)

Replace(
    'src/Game/Game.cpp',
    '#include <unordered_set>\n',
    '#include <unordered_set>\n#include <utility>\n'
)

Replace(
    'src/Game/Game.cpp',
    '''    const glm::vec3 ExitRight{\n        ExitForward.z,''',
    '''    if (!GameRenderer.HasExitDoorModel())\n    {\n        const glm::vec3 ExitRight{\n        ExitForward.z,'''
)

Replace(
    'src/Game/Game.cpp',
    '''    if (!GameRenderer.HasEntityModels())\n    {''',
    '''    }\n\n    if (!GameRenderer.HasEntityModels())\n    {'''
)

Replace(
    'src/Game/Game.cpp',
    '''    if (\n        Hunter.IsActive() &&\n        GameRenderer.HasEntityModels()\n    )''',
    '''    if (GameRenderer.HasExitDoorModel())\n    {\n        GameRenderer.DrawExitDoor(\n            ExitPosition,\n            ExitForward\n        );\n    }\n\n    if (\n        Hunter.IsActive() &&\n        GameRenderer.HasEntityModels()\n    )'''
)

Replace(
    'src/Rendering/Renderer.h',
    '''    bool HasBreakerModel() const;\n    void DrawBreaker(\n        const glm::vec3& Position,\n        const glm::vec3& Forward\n    );\n\n    bool HasEntityModels() const;''',
    '''    bool HasBreakerModel() const;\n    void DrawBreaker(\n        const glm::vec3& Position,\n        const glm::vec3& Forward\n    );\n\n    bool HasExitDoorModel() const;\n    void DrawExitDoor(\n        const glm::vec3& Position,\n        const glm::vec3& Forward\n    );\n\n    bool HasEntityModels() const;'''
)

Replace(
    'src/Rendering/Renderer.h',
    '''    EntityModel BreakerModel;\n    EntityModel GhostEntityModel;''',
    '''    EntityModel BreakerModel;\n    EntityModel ExitDoorModel;\n    EntityModel GhostEntityModel;'''
)

Replace(
    'src/Rendering/Renderer.cpp',
    '''    BreakerModel.Load(\n        "assets/models/power_box_01/power_box_01_1k.gltf",\n        1.05f\n    );\n\n    GhostEntityModel.Load(''',
    '''    BreakerModel.Load(\n        "assets/models/power_box_01/power_box_01_1k.gltf",\n        0.92f\n    );\n\n    ExitDoorModel.Load(\n        "assets/models/exit-door.glb",\n        2.34f\n    );\n\n    GhostEntityModel.Load('''
)

Replace(
    'src/Rendering/Renderer.cpp',
    '''    BreakerModel.Shutdown();\n    GhostEntityModel.Shutdown();''',
    '''    BreakerModel.Shutdown();\n    ExitDoorModel.Shutdown();\n    GhostEntityModel.Shutdown();'''
)

Replace(
    'src/Rendering/Renderer.cpp',
    '''bool Renderer::HasEntityModels() const\n{''',
    '''bool Renderer::HasExitDoorModel() const\n{\n    return ExitDoorModel.IsReady();\n}\n\nvoid Renderer::DrawExitDoor(\n    const glm::vec3& Position,\n    const glm::vec3& Forward\n)\n{\n    if (!ExitDoorModel.IsReady())\n        return;\n\n    ExitDoorModel.Draw(\n        View,\n        Projection,\n        CameraPosition,\n        Position,\n        Forward,\n        glm::vec3{1.0f},\n        ActiveLightPositions,\n        ActiveLightColors,\n        ActiveLightCount\n    );\n}\n\nbool Renderer::HasEntityModels() const\n{'''
)

Replace(
    'src/Rendering/Renderer.cpp',
    '    float FogAmount = smoothstep(26.0, 72.0, DistanceToCamera);',
    '    float FogAmount = smoothstep(24.0, 60.0, DistanceToCamera);'
)

Replace(
    'src/Rendering/EntityModel.cpp',
    '#include <cstdint>\n#include <iostream>\n',
    '#include <cstdint>\n#include <filesystem>\n#include <fstream>\n#include <iostream>\n'
)

Replace(
    'src/Rendering/EntityModel.cpp',
    '''    if (!CreateShader())\n        return false;\n\n    cgltf_options Options{};''',
    '''    if (!CreateShader())\n        return false;\n\n    const std::filesystem::path ModelDirectory =\n        std::filesystem::path(Path).parent_path();\n\n    cgltf_options Options{};'''
)

OldTextureBlock = '''                    if (\n                        Texture != nullptr &&\n                        Texture->image != nullptr &&\n                        Texture->image->buffer_view != nullptr &&\n                        Texture->image->buffer_view->buffer != nullptr &&\n                        Texture->image->buffer_view->buffer->data != nullptr\n                    )\n                    {\n                        const cgltf_buffer_view& View =\n                            *Texture->image->buffer_view;\n\n                        const unsigned char* ImageData =\n                            static_cast<const unsigned char*>(\n                                View.buffer->data\n                            ) +\n                            View.offset;\n\n                        TargetPrimitive.Texture =\n                            CreateTexture(\n                                ImageData,\n                                static_cast<std::size_t>(\n                                    View.size\n                                )\n                            );\n                    }'''

NewTextureBlock = '''                    if (\n                        Texture != nullptr &&\n                        Texture->image != nullptr\n                    )\n                    {\n                        const cgltf_image& Image =\n                            *Texture->image;\n\n                        if (\n                            Image.buffer_view != nullptr &&\n                            Image.buffer_view->buffer != nullptr &&\n                            Image.buffer_view->buffer->data != nullptr\n                        )\n                        {\n                            const cgltf_buffer_view& View =\n                                *Image.buffer_view;\n\n                            const unsigned char* ImageData =\n                                static_cast<const unsigned char*>(\n                                    View.buffer->data\n                                ) +\n                                View.offset;\n\n                            TargetPrimitive.Texture =\n                                CreateTexture(\n                                    ImageData,\n                                    static_cast<std::size_t>(\n                                        View.size\n                                    )\n                                );\n                        }\n                        else if (\n                            Image.uri != nullptr &&\n                            Image.uri[0] != '\\0'\n                        )\n                        {\n                            const std::filesystem::path ImagePath =\n                                ModelDirectory /\n                                std::filesystem::path(Image.uri);\n\n                            std::ifstream ImageFile(\n                                ImagePath,\n                                std::ios::binary\n                            );\n\n                            if (ImageFile.is_open())\n                            {\n                                std::vector<unsigned char> Bytes(\n                                    std::istreambuf_iterator<char>(ImageFile),\n                                    std::istreambuf_iterator<char>()\n                                );\n\n                                if (!Bytes.empty())\n                                {\n                                    TargetPrimitive.Texture =\n                                        CreateTexture(\n                                            Bytes.data(),\n                                            Bytes.size()\n                                        );\n                                }\n                            }\n                        }\n                    }'''

Replace('src/Rendering/EntityModel.cpp', OldTextureBlock, NewTextureBlock)

Replace(
    'src/Rendering/EntityModel.cpp',
    '#include <limits>\n#include <vector>\n',
    '#include <iterator>\n#include <limits>\n#include <vector>\n'
)

Replace(
    'src/Audio/AudioSystem.h',
    '''    void PlayBreaker(const glm::vec3& Position);\n    void PlayFootstep''',
    '''    void PlayBreaker(const glm::vec3& Position);\n    void PlayEntityRelease(const glm::vec3& Position);\n    void PlayFootstep'''
)

Replace(
    'src/Audio/AudioSystem.cpp',
    '''    EntityMetal = LoadSound(\n        "assets/audio/entity-metal.wav",\n        false,\n        true,\n        0.25f\n    );''',
    '''    EntityMetal = LoadSound(\n        "assets/audio/entity-metal.wav",\n        false,\n        true,\n        0.48f\n    );\n\n    if (EntityMetal != nullptr)\n    {\n        ma_sound_set_min_distance(EntityMetal, 2.0f);\n        ma_sound_set_max_distance(EntityMetal, 86.0f);\n        ma_sound_set_rolloff(EntityMetal, 0.42f);\n    }'''
)

Replace(
    'src/Audio/AudioSystem.cpp',
    '''    EntityLaugh = LoadSound(\n        "assets/audio/entity-laugh.wav",\n        false,\n        true,\n        0.24f\n    );''',
    '''    EntityLaugh = LoadSound(\n        "assets/audio/entity-laugh.wav",\n        false,\n        true,\n        0.46f\n    );\n\n    if (EntityLaugh != nullptr)\n    {\n        ma_sound_set_min_distance(EntityLaugh, 2.0f);\n        ma_sound_set_max_distance(EntityLaugh, 78.0f);\n        ma_sound_set_rolloff(EntityLaugh, 0.48f);\n    }'''
)

Replace(
    'src/Audio/AudioSystem.cpp',
    '    EntityCueTimer = 3.5f;',
    '    EntityCueTimer = 0.85f;'
)

OldCue = '''    if (EntityMetal != nullptr && EntityActive)\n    {\n        ma_sound_set_position(\n            EntityMetal,\n            EntityPosition.x,\n            EntityPosition.y + 0.9f,\n            EntityPosition.z\n        );\n\n        EntityCueTimer -= std::max(DeltaTime, 0.0f);\n\n        if (\n            EntityCueTimer <= 0.0f &&\n            Distance < 42.0f\n        )\n        {\n            const float Pitch =\n                0.82f +\n                static_cast<float>(EntityCueIndex % 4) * 0.055f;\n\n            RestartSpatial(\n                EntityMetal,\n                EntityPosition + glm::vec3{0.0f, 0.8f, 0.0f},\n                0.12f + Threat * 0.24f,\n                Pitch\n            );\n\n            ++EntityCueIndex;\n\n            EntityCueTimer =\n                6.2f -\n                Threat * 3.7f +\n                static_cast<float>(EntityCueIndex % 3) * 0.55f;\n        }\n    }\n    else\n    {\n        EntityCueTimer = 2.8f;\n    }'''

NewCue = '''    if (EntityActive)\n    {\n        EntityCueTimer -= std::max(DeltaTime, 0.0f);\n\n        if (\n            EntityCueTimer <= 0.0f &&\n            Distance < 92.0f\n        )\n        {\n            const float Pitch =\n                0.86f +\n                static_cast<float>(EntityCueIndex % 5) * 0.035f;\n\n            if (\n                EntityLaugh != nullptr &&\n                EntityCueIndex % 3 == 2\n            )\n            {\n                RestartSpatial(\n                    EntityLaugh,\n                    EntityPosition + glm::vec3{0.0f, 1.25f, 0.0f},\n                    0.44f + Threat * 0.22f,\n                    0.91f + Threat * 0.04f\n                );\n            }\n            else\n            {\n                RestartSpatial(\n                    EntityMetal,\n                    EntityPosition + glm::vec3{0.0f, 0.8f, 0.0f},\n                    0.42f + Threat * 0.32f,\n                    Pitch\n                );\n            }\n\n            ++EntityCueIndex;\n\n            EntityCueTimer =\n                4.4f -\n                Threat * 2.0f +\n                static_cast<float>(EntityCueIndex % 3) * 0.38f;\n        }\n    }\n    else\n    {\n        EntityCueTimer = 0.85f;\n    }'''

Replace('src/Audio/AudioSystem.cpp', OldCue, NewCue)

Replace(
    'src/Audio/AudioSystem.cpp',
    '''void AudioSystem::PlayBreaker(const glm::vec3& Position)\n{''',
    '''void AudioSystem::PlayEntityRelease(const glm::vec3& Position)\n{\n    RestartSpatial(\n        EntityMetal,\n        Position + glm::vec3{0.0f, 0.9f, 0.0f},\n        0.78f,\n        0.86f\n    );\n\n    EntityCueTimer = 1.25f;\n}\n\nvoid AudioSystem::PlayBreaker(const glm::vec3& Position)\n{'''
)

Replace(
    'src/Core/Version.h',
    'inline constexpr const char* Text = "0.3.24";',
    'inline constexpr const char* Text = "0.3.25";'
)

BackroomsRc = Read('src/Platform/Windows/Backrooms.rc')
BackroomsRc = BackroomsRc.replace('0,3,24,0', '0,3,25,0')
BackroomsRc = BackroomsRc.replace('0.3.24', '0.3.25')
Write('src/Platform/Windows/Backrooms.rc', BackroomsRc)

Gltf = Read('assets/models/power_box_01/power_box_01_1k.gltf')
Gltf = Gltf.replace(
    '"rotation":[0,-0.8152777552604675,0,0.5790701508522034]',
    '"rotation":[0,-0.48480962024633706,0,0.8746197071393957]'
)
Gltf = Gltf.replace(
    '"baseColorFactor":[0.18,0.19,0.17,1.0]',
    '"baseColorFactor":[1.0,1.0,1.0,1.0]'
)
Write('assets/models/power_box_01/power_box_01_1k.gltf', Gltf)

Write(
    'update/release_notes.txt',
    'V0.3.25 replaces the procedural-looking exit with a real authored GLB door, restores the real Poly Haven breaker textures and changes the breaker door angle so the switches and wiring are visible, upgrades the monster encounter to audible long-range positional cues, and replaces the fixed 15x15 maze with deterministic streamed Backrooms chunks that rebuild around the player while fog hides the streaming frontier.\n'
)

Credits = Read('THIRD_PARTY_ASSETS.md')
Credits = Credits.replace(
    '  - The native renderer uses the real mesh geometry and a neutral metal base color; external texture files are not required by this build.',
    '  - V0.3.25 ships the matching 1K diffuse, normal, and ARM texture files and teaches the native glTF loader to resolve external image URIs. The breaker door rotation is reduced so the switches and wiring remain readable from the hallway.'
)

if '## Exit door model' not in Credits:
    Credits += '''\n\n## Exit door model\n\n- `assets/models/exit-door.glb`\n  - Source: `spongebobtdgameplay-prog/The-Infinity-Store`, `Models/Architecture/GLB/Door_3.glb`.\n  - Usage: authored Level 0 exit-door mesh replacing the normal procedural box-built exit visual.\n  - This is shared project content from the user's Infinity Store repository.\n'''

Write('THIRD_PARTY_ASSETS.md', Credits)

Checks = [
    ('src/Core/Version.h', '0.3.25'),
    ('src/World/WorldGenerator.cpp', 'BuildAround'),
    ('src/World/WorldGenerator.cpp', 'CreateChunkCells'),
    ('src/Game/Game.cpp', 'Streamer.NeedsRebuild'),
    ('src/Game/Game.cpp', 'DrawExitDoor'),
    ('src/Rendering/Renderer.cpp', 'ExitDoorModel.Load'),
    ('src/Rendering/EntityModel.cpp', 'ModelDirectory /'),
    ('src/Audio/AudioSystem.cpp', 'PlayEntityRelease'),
]

for PathName, Needle in Checks:
    if Needle not in Read(PathName):
        raise RuntimeError(f'{PathName}: missing expected marker {Needle}')

print('V0.3.25 source patch prepared successfully')
