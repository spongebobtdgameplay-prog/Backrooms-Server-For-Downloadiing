#include "Entity.h"

#include "../Physics/Collision.h"
#include "../World/WorldGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

void Entity::Reset(const glm::vec3& StartPosition, uint32_t WorldSeed)
{
    EntityPosition = StartPosition;
    EntityPosition.y = 0.0f;
    Direction = {0.0f, 0.0f, -1.0f};
    Seed = WorldSeed == 0 ? 1 : WorldSeed;
    RepathTimer = 0.0f;
    StuckTimer = 0.0f;
    ShiftTimer = 7.0f;
    ShiftProgress = 1.0f;
    ReleaseGraceTimer = 0.0f;
    EncounterAge = 0.0f;
    Active = false;
    DemonForm = false;
    PreviousDemonForm = false;
    ShiftedThisFrame = false;
    TargetWorldX = 0;
    TargetWorldZ = 0;
    Path.clear();
    PathIndex = 0;
}

void Entity::Release()
{
    Active = true;
    RepathTimer = 0.0f;
    StuckTimer = 0.0f;
    ReleaseGraceTimer = 0.45f;
    EncounterAge = 0.0f;
}

float Entity::DistanceTo(const glm::vec3& Point) const
{
    const float DX = Point.x - EntityPosition.x;
    const float DZ = Point.z - EntityPosition.z;
    return std::sqrt(DX * DX + DZ * DZ);
}

int Entity::CellIndex(
    const glm::vec3& Position,
    const WorldData& World
) const
{
    const int WorldX = static_cast<int>(std::round(Position.x / World.CellSize));
    const int WorldZ = static_cast<int>(std::round(Position.z / World.CellSize));
    const int LocalX = std::clamp(WorldX - World.OriginCellX, 0, World.Columns - 1);
    const int LocalZ = std::clamp(WorldZ - World.OriginCellZ, 0, World.Rows - 1);
    return LocalZ * World.Columns + LocalX;
}

std::vector<int> Entity::Neighbors(int Index, const WorldData& World) const
{
    const MazeCell& Cell = World.Cells[static_cast<std::size_t>(Index)];
    const int LocalX = Index % World.Columns;
    const int LocalZ = Index / World.Columns;
    std::vector<int> Result;
    if (!Cell.Walls[0] && LocalZ > 0) Result.push_back(Index - World.Columns);
    if (!Cell.Walls[1] && LocalX < World.Columns - 1) Result.push_back(Index + 1);
    if (!Cell.Walls[2] && LocalZ < World.Rows - 1) Result.push_back(Index + World.Columns);
    if (!Cell.Walls[3] && LocalX > 0) Result.push_back(Index - 1);
    return Result;
}

void Entity::BuildPath(const glm::vec3& Target, const WorldData& CollisionWorld)
{
    const glm::vec2 Start2{EntityPosition.x, EntityPosition.z};
    const glm::vec2 Target2{Target.x, Target.z};
    const glm::vec2 Midpoint = (Start2 + Target2) * 0.5f;
    const float Span = std::max(
        std::abs(Target2.x - Start2.x),
        std::abs(Target2.y - Start2.y)
    );
    const float ChunkMeters = std::max(
        CollisionWorld.CellSize * static_cast<float>(CollisionWorld.ChunkCells),
        1.0f
    );
    const int Radius = std::clamp(
        static_cast<int>(std::ceil(Span * 0.5f / ChunkMeters)) + 3,
        1,
        16
    );

    WorldGenerator Generator(Seed);
    const WorldData NavigationWorld = Generator.BuildMapRegion(
        {Midpoint.x, 0.0f, Midpoint.y},
        Radius
    );

    const int Start = CellIndex(EntityPosition, NavigationWorld);
    const int Goal = CellIndex(Target, NavigationWorld);
    TargetWorldX = static_cast<int>(std::round(Target.x / NavigationWorld.CellSize));
    TargetWorldZ = static_cast<int>(std::round(Target.z / NavigationWorld.CellSize));

    if (Start == Goal)
    {
        Path = {{Target.x, 0.0f, Target.z}};
        PathIndex = 0;
        return;
    }

    const int Count = NavigationWorld.Columns * NavigationWorld.Rows;
    std::vector<float> G(static_cast<std::size_t>(Count), std::numeric_limits<float>::infinity());
    std::vector<float> F(static_cast<std::size_t>(Count), std::numeric_limits<float>::infinity());
    std::vector<int> Parent(static_cast<std::size_t>(Count), -1);

    struct Node
    {
        int Index = -1;
        float Score = 0.0f;
        bool operator<(const Node& Other) const { return Score > Other.Score; }
    };

    const MazeCell& GoalCell = NavigationWorld.Cells[static_cast<std::size_t>(Goal)];
    auto Heuristic = [&](int Index)
    {
        const MazeCell& Cell = NavigationWorld.Cells[static_cast<std::size_t>(Index)];
        return static_cast<float>(
            std::abs(Cell.X - GoalCell.X) +
            std::abs(Cell.Z - GoalCell.Z)
        );
    };

    std::priority_queue<Node> Open;
    G[static_cast<std::size_t>(Start)] = 0.0f;
    F[static_cast<std::size_t>(Start)] = Heuristic(Start);
    Open.push({Start, F[static_cast<std::size_t>(Start)]});
    bool Found = false;

    while (!Open.empty())
    {
        const Node CurrentNode = Open.top();
        Open.pop();
        const int Current = CurrentNode.Index;
        if (Current == Goal)
        {
            Found = true;
            break;
        }

        for (int Next : Neighbors(Current, NavigationWorld))
        {
            const float Tentative = G[static_cast<std::size_t>(Current)] + 1.0f;
            if (Tentative >= G[static_cast<std::size_t>(Next)])
                continue;
            Parent[static_cast<std::size_t>(Next)] = Current;
            G[static_cast<std::size_t>(Next)] = Tentative;
            F[static_cast<std::size_t>(Next)] = Tentative + Heuristic(Next);
            Open.push({Next, F[static_cast<std::size_t>(Next)]});
        }
    }

    Path.clear();
    PathIndex = 0;
    if (!Found)
        return;

    std::vector<int> CellPath;
    int Cursor = Goal;
    while (Cursor != -1)
    {
        CellPath.push_back(Cursor);
        if (Cursor == Start)
            break;
        Cursor = Parent[static_cast<std::size_t>(Cursor)];
    }

    std::reverse(CellPath.begin(), CellPath.end());
    if (CellPath.size() > 1)
        CellPath.erase(CellPath.begin());

    Path.reserve(CellPath.size() + 1);
    for (int Index : CellPath)
    {
        const MazeCell& Cell = NavigationWorld.Cells[static_cast<std::size_t>(Index)];
        Path.push_back({
            static_cast<float>(Cell.X) * NavigationWorld.CellSize,
            0.0f,
            static_cast<float>(Cell.Z) * NavigationWorld.CellSize
        });
    }
    Path.push_back({Target.x, 0.0f, Target.z});
}

bool Entity::Update(float DeltaTime, const glm::vec3& PlayerPosition, const WorldData& World)
{
    ShiftedThisFrame = false;
    if (!Active)
        return false;

    const float SafeDelta = std::max(DeltaTime, 0.0f);
    EncounterAge += SafeDelta;
    ReleaseGraceTimer = std::max(0.0f, ReleaseGraceTimer - SafeDelta);

    const float DistanceBeforeMove = DistanceTo(PlayerPosition);
    const float ChaseBlend = std::clamp(
        1.0f - (DistanceBeforeMove - 7.0f) / 30.0f,
        0.0f,
        1.0f
    );
    float MoveSpeed = 3.75f + ChaseBlend * 2.55f;
    if (ReleaseGraceTimer > 0.0f)
        MoveSpeed = std::min(MoveSpeed, 2.65f);
    if (DemonForm && ReleaseGraceTimer <= 0.0f)
        MoveSpeed += 0.45f;

    RepathTimer -= SafeDelta;
    const int PlayerWorldX = static_cast<int>(std::round(PlayerPosition.x / World.CellSize));
    const int PlayerWorldZ = static_cast<int>(std::round(PlayerPosition.z / World.CellSize));

    if (
        RepathTimer <= 0.0f ||
        PlayerWorldX != TargetWorldX ||
        PlayerWorldZ != TargetWorldZ ||
        PathIndex >= Path.size()
    )
    {
        RepathTimer = RepathInterval;
        BuildPath(PlayerPosition, World);
    }

    if (PathIndex < Path.size())
    {
        glm::vec3 Target = Path[PathIndex];
        Target.y = 0.0f;
        glm::vec3 Delta = Target - EntityPosition;
        Delta.y = 0.0f;
        float Distance = glm::length(Delta);

        while (Distance < 0.72f && PathIndex + 1 < Path.size())
        {
            ++PathIndex;
            Target = Path[PathIndex];
            Delta = Target - EntityPosition;
            Delta.y = 0.0f;
            Distance = glm::length(Delta);
        }

        if (Distance > 0.001f)
        {
            Direction = Delta / Distance;
            const float Step = std::min(MoveSpeed * SafeDelta, Distance);
            const glm::vec3 Desired = Direction * Step;
            const glm::vec3 Previous = EntityPosition;
            EntityPosition = Collision::ResolveHorizontalMove(
                EntityPosition,
                Desired,
                Radius,
                World.Colliders
            );
            const glm::vec3 Actual = EntityPosition - Previous;
            const float DesiredSq = glm::dot(Desired, Desired);
            const float ActualSq = glm::dot(Actual, Actual);

            if (DesiredSq > 0.000001f && ActualSq < DesiredSq * 0.08f)
            {
                StuckTimer += SafeDelta;
                RepathTimer = 0.0f;
                if (StuckTimer > 0.38f && PathIndex + 1 < Path.size())
                {
                    ++PathIndex;
                    StuckTimer = 0.0f;
                }
            }
            else
            {
                StuckTimer = std::max(0.0f, StuckTimer - SafeDelta * 3.0f);
            }
        }
    }

    const float Distance = DistanceTo(PlayerPosition);
    ShiftTimer -= SafeDelta;
    bool DesiredDemon = DemonForm;
    if (Distance < 9.0f)
        DesiredDemon = true;
    else if (Distance > 16.0f)
        DesiredDemon = false;
    else if (ShiftTimer <= 0.0f)
        DesiredDemon = !DemonForm;

    if (DesiredDemon != DemonForm)
    {
        PreviousDemonForm = DemonForm;
        DemonForm = DesiredDemon;
        ShiftProgress = 0.0f;
        ShiftTimer = 6.0f;
        ShiftedThisFrame = true;
    }

    ShiftProgress = std::min(1.0f, ShiftProgress + SafeDelta / 0.42f);
    return ReleaseGraceTimer <= 0.0f && DistanceTo(PlayerPosition) < 0.78f;
}

std::vector<SceneBox> Entity::BuildRenderBoxes() const
{
    return {};
}
