#include "MapNavigation.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace
{
    int WorldCell(float Coordinate, float CellSize)
    {
        return static_cast<int>(std::round(Coordinate / CellSize));
    }

    float Heuristic(int X, int Z, int GoalX, int GoalZ)
    {
        return static_cast<float>(
            std::abs(GoalX - X) + std::abs(GoalZ - Z)
        );
    }
}

std::vector<glm::vec2> MapNavigation::FindPath(
    const WorldData& World,
    const glm::vec2& Start,
    const glm::vec2& Goal
)
{
    std::vector<glm::vec2> Empty;

    if (
        World.Cells.empty() ||
        World.Columns <= 0 ||
        World.Rows <= 0 ||
        World.CellSize <= 0.0f
    )
    {
        return Empty;
    }

    const int StartWorldX = WorldCell(Start.x, World.CellSize);
    const int StartWorldZ = WorldCell(Start.y, World.CellSize);
    const int GoalWorldX = WorldCell(Goal.x, World.CellSize);
    const int GoalWorldZ = WorldCell(Goal.y, World.CellSize);

    if (
        !World.ContainsWorldCell(StartWorldX, StartWorldZ) ||
        !World.ContainsWorldCell(GoalWorldX, GoalWorldZ)
    )
    {
        return Empty;
    }

    const int StartX = StartWorldX - World.OriginCellX;
    const int StartZ = StartWorldZ - World.OriginCellZ;
    const int GoalX = GoalWorldX - World.OriginCellX;
    const int GoalZ = GoalWorldZ - World.OriginCellZ;

    const int CellCount = World.Columns * World.Rows;
    const int StartIndex = StartZ * World.Columns + StartX;
    const int GoalIndex = GoalZ * World.Columns + GoalX;

    std::vector<float> Cost(
        static_cast<std::size_t>(CellCount),
        std::numeric_limits<float>::infinity()
    );

    std::vector<int> CameFrom(
        static_cast<std::size_t>(CellCount),
        -1
    );

    std::vector<bool> Closed(
        static_cast<std::size_t>(CellCount),
        false
    );

    using OpenEntry = std::pair<float, int>;
    std::priority_queue<
        OpenEntry,
        std::vector<OpenEntry>,
        std::greater<OpenEntry>
    > Open;

    Cost[static_cast<std::size_t>(StartIndex)] = 0.0f;
    Open.push({
        Heuristic(StartX, StartZ, GoalX, GoalZ),
        StartIndex
    });

    constexpr int DeltaX[4] = {0, 1, 0, -1};
    constexpr int DeltaZ[4] = {-1, 0, 1, 0};

    while (!Open.empty())
    {
        const int CurrentIndex = Open.top().second;
        Open.pop();

        if (Closed[static_cast<std::size_t>(CurrentIndex)])
            continue;

        Closed[static_cast<std::size_t>(CurrentIndex)] = true;

        if (CurrentIndex == GoalIndex)
            break;

        const int CurrentX = CurrentIndex % World.Columns;
        const int CurrentZ = CurrentIndex / World.Columns;
        const MazeCell& Current = World.Cell(CurrentX, CurrentZ);

        for (int DirectionIndex = 0; DirectionIndex < 4; ++DirectionIndex)
        {
            if (Current.Walls[static_cast<std::size_t>(DirectionIndex)])
                continue;

            const int NextX = CurrentX + DeltaX[DirectionIndex];
            const int NextZ = CurrentZ + DeltaZ[DirectionIndex];

            if (
                NextX < 0 ||
                NextZ < 0 ||
                NextX >= World.Columns ||
                NextZ >= World.Rows
            )
            {
                continue;
            }

            const int NextIndex = NextZ * World.Columns + NextX;

            if (Closed[static_cast<std::size_t>(NextIndex)])
                continue;

            const float NewCost =
                Cost[static_cast<std::size_t>(CurrentIndex)] + 1.0f;

            if (NewCost >= Cost[static_cast<std::size_t>(NextIndex)])
                continue;

            Cost[static_cast<std::size_t>(NextIndex)] = NewCost;
            CameFrom[static_cast<std::size_t>(NextIndex)] = CurrentIndex;

            const float Priority =
                NewCost + Heuristic(NextX, NextZ, GoalX, GoalZ);

            Open.push({Priority, NextIndex});
        }
    }

    if (
        GoalIndex != StartIndex &&
        CameFrom[static_cast<std::size_t>(GoalIndex)] < 0
    )
    {
        return Empty;
    }

    std::vector<int> Reversed;
    int Cursor = GoalIndex;
    Reversed.push_back(Cursor);

    while (Cursor != StartIndex)
    {
        Cursor = CameFrom[static_cast<std::size_t>(Cursor)];

        if (Cursor < 0)
            return Empty;

        Reversed.push_back(Cursor);
    }

    std::reverse(Reversed.begin(), Reversed.end());

    std::vector<glm::vec2> Raw;
    Raw.reserve(Reversed.size() + 2);
    Raw.push_back(Start);

    for (int Index : Reversed)
    {
        const int LocalX = Index % World.Columns;
        const int LocalZ = Index / World.Columns;
        const MazeCell& Cell = World.Cell(LocalX, LocalZ);

        Raw.push_back({
            static_cast<float>(Cell.X) * World.CellSize,
            static_cast<float>(Cell.Z) * World.CellSize
        });
    }

    Raw.push_back(Goal);

    if (Raw.size() <= 3)
        return Raw;

    std::vector<glm::vec2> Simplified;
    Simplified.reserve(Raw.size());
    Simplified.push_back(Raw.front());

    for (std::size_t I = 1; I + 1 < Raw.size(); ++I)
    {
        const glm::vec2 Before = Raw[I] - Raw[I - 1];
        const glm::vec2 After = Raw[I + 1] - Raw[I];

        const int BeforeX = Before.x > 0.1f ? 1 : (Before.x < -0.1f ? -1 : 0);
        const int BeforeZ = Before.y > 0.1f ? 1 : (Before.y < -0.1f ? -1 : 0);
        const int AfterX = After.x > 0.1f ? 1 : (After.x < -0.1f ? -1 : 0);
        const int AfterZ = After.y > 0.1f ? 1 : (After.y < -0.1f ? -1 : 0);

        if (BeforeX != AfterX || BeforeZ != AfterZ)
            Simplified.push_back(Raw[I]);
    }

    Simplified.push_back(Raw.back());
    return Simplified;
}
