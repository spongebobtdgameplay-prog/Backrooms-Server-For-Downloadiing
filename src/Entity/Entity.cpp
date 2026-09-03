#include "Entity.h"

#include "../Physics/Collision.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

void Entity::Reset(const glm::vec3& StartPosition)
{
    EntityPosition = StartPosition;
    EntityPosition.y = 0.0f;

    Direction = {0.0f, 0.0f, -1.0f};

    RepathTimer = 0.0f;
    ShiftTimer = 7.0f;
    ShiftProgress = 1.0f;

    Active = false;
    DemonForm = false;
    ShiftedThisFrame = false;

    TargetCell = -1;
    Path.clear();
    PathIndex = 0;
}

void Entity::Release()
{
    Active = true;
    RepathTimer = 0.0f;
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
    const int X = std::clamp(
        static_cast<int>(std::round(Position.x / World.CellSize)),
        0,
        World.Columns - 1
    );

    const int Z = std::clamp(
        static_cast<int>(std::round(Position.z / World.CellSize)),
        0,
        World.Rows - 1
    );

    return Z * World.Columns + X;
}

std::vector<int> Entity::Neighbors(
    int Index,
    const WorldData& World
) const
{
    const MazeCell& Cell =
        World.Cells[static_cast<std::size_t>(Index)];

    std::vector<int> Result;

    if (!Cell.Walls[0] && Cell.Z > 0)
        Result.push_back((Cell.Z - 1) * World.Columns + Cell.X);

    if (!Cell.Walls[1] && Cell.X < World.Columns - 1)
        Result.push_back(Cell.Z * World.Columns + Cell.X + 1);

    if (!Cell.Walls[2] && Cell.Z < World.Rows - 1)
        Result.push_back((Cell.Z + 1) * World.Columns + Cell.X);

    if (!Cell.Walls[3] && Cell.X > 0)
        Result.push_back(Cell.Z * World.Columns + Cell.X - 1);

    return Result;
}

void Entity::BuildPath(
    const glm::vec3& Target,
    const WorldData& World
)
{
    const int Start = CellIndex(EntityPosition, World);
    const int Goal = CellIndex(Target, World);

    TargetCell = Goal;

    if (Start == Goal)
    {
        Path = {{Target.x, 0.0f, Target.z}};
        PathIndex = 0;
        return;
    }

    const int Count = World.Columns * World.Rows;

    std::vector<float> G(
        static_cast<std::size_t>(Count),
        std::numeric_limits<float>::infinity()
    );

    std::vector<float> F(
        static_cast<std::size_t>(Count),
        std::numeric_limits<float>::infinity()
    );

    std::vector<int> Parent(
        static_cast<std::size_t>(Count),
        -1
    );

    struct Node
    {
        int Index = -1;
        float Score = 0.0f;

        bool operator<(const Node& Other) const
        {
            return Score > Other.Score;
        }
    };

    const MazeCell& GoalCell =
        World.Cells[static_cast<std::size_t>(Goal)];

    auto Heuristic = [&](int Index)
    {
        const MazeCell& Cell =
            World.Cells[static_cast<std::size_t>(Index)];

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

        for (int Next : Neighbors(Current, World))
        {
            const float Tentative =
                G[static_cast<std::size_t>(Current)] + 1.0f;

            if (Tentative >= G[static_cast<std::size_t>(Next)])
                continue;

            Parent[static_cast<std::size_t>(Next)] = Current;
            G[static_cast<std::size_t>(Next)] = Tentative;
            F[static_cast<std::size_t>(Next)] =
                Tentative + Heuristic(Next);

            Open.push({
                Next,
                F[static_cast<std::size_t>(Next)]
            });
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

    for (int Index : CellPath)
    {
        const MazeCell& Cell =
            World.Cells[static_cast<std::size_t>(Index)];

        Path.push_back({
            static_cast<float>(Cell.X) * World.CellSize,
            0.0f,
            static_cast<float>(Cell.Z) * World.CellSize
        });
    }

    Path.push_back({Target.x, 0.0f, Target.z});
}

bool Entity::Update(
    float DeltaTime,
    const glm::vec3& PlayerPosition,
    const WorldData& World
)
{
    ShiftedThisFrame = false;

    if (!Active)
        return false;

    RepathTimer -= DeltaTime;

    const int PlayerCell = CellIndex(PlayerPosition, World);

    if (
        RepathTimer <= 0.0f ||
        PlayerCell != TargetCell ||
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

        while (
            Distance < 0.42f &&
            PathIndex + 1 < Path.size()
        )
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

            const float Step =
                std::min(Speed * DeltaTime, Distance);

            const glm::vec3 Desired =
                Direction * Step;

            const glm::vec3 Previous =
                EntityPosition;

            EntityPosition =
                Collision::ResolveHorizontalMove(
                    EntityPosition,
                    Desired,
                    Radius,
                    World.Colliders
                );

            const glm::vec3 Actual =
                EntityPosition - Previous;

            if (
                glm::dot(Actual, Actual) <
                glm::dot(Desired, Desired) * 0.08f
            )
            {
                RepathTimer = 0.0f;
            }
        }
    }

    const float Distance = DistanceTo(PlayerPosition);

    ShiftTimer -= DeltaTime;

    bool DesiredDemon = DemonForm;

    if (Distance < 10.5f)
        DesiredDemon = true;
    else if (Distance > 15.5f)
        DesiredDemon = false;
    else if (ShiftTimer <= 0.0f)
        DesiredDemon = !DemonForm;

    if (DesiredDemon != DemonForm)
    {
        DemonForm = DesiredDemon;
        ShiftProgress = 0.0f;
        ShiftTimer = 6.0f;
        ShiftedThisFrame = true;
    }

    ShiftProgress = std::min(
        1.0f,
        ShiftProgress + DeltaTime / 0.42f
    );

    return DistanceTo(PlayerPosition) < 0.9f;
}

std::vector<SceneBox> Entity::BuildRenderBoxes() const
{
    std::vector<SceneBox> Boxes;

    if (!Active)
        return Boxes;

    const float Ease =
        ShiftProgress * ShiftProgress *
        (3.0f - 2.0f * ShiftProgress);

    const float HeightScale =
        0.58f + Ease * 0.42f;

    const glm::vec3 Color =
        DemonForm
            ? glm::vec3{0.22f, 0.045f, 0.025f}
            : glm::vec3{0.055f, 0.052f, 0.043f};

    const glm::vec3 Emissive =
        DemonForm
            ? glm::vec3{0.025f, 0.002f, 0.001f}
            : glm::vec3{0.003f, 0.003f, 0.002f};

    Boxes.push_back({
        EntityPosition + glm::vec3{0.0f, 1.0f * HeightScale, 0.0f},
        {0.52f, 1.55f * HeightScale, 0.38f},
        Color,
        Emissive,
        0.88f
    });

    Boxes.push_back({
        EntityPosition + glm::vec3{0.0f, 1.92f * HeightScale, 0.0f},
        {0.43f, 0.43f, 0.43f},
        Color * 0.86f,
        Emissive,
        0.91f
    });

    Boxes.push_back({
        EntityPosition + glm::vec3{-0.38f, 1.12f * HeightScale, 0.0f},
        {0.18f, 1.15f * HeightScale, 0.18f},
        Color,
        Emissive,
        0.9f
    });

    Boxes.push_back({
        EntityPosition + glm::vec3{0.38f, 1.12f * HeightScale, 0.0f},
        {0.18f, 1.15f * HeightScale, 0.18f},
        Color,
        Emissive,
        0.9f
    });

    return Boxes;
}
