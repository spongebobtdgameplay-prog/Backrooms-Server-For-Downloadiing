#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace
{
    glm::vec3 LevelZeroYellow()
    {
        return {200.0f / 255.0f, 187.0f / 255.0f, 97.0f / 255.0f};
    }

    glm::vec3 LevelZeroInk()
    {
        return {39.0f / 255.0f, 35.0f / 255.0f, 15.0f / 255.0f};
    }

    float FullMapScale(float Zoom)
    {
        return 7.2f * std::clamp(Zoom, 0.45f, 4.0f);
    }

    std::string MarkerName(MapMarkerKind Kind)
    {
        if (Kind == MapMarkerKind::Breaker)
            return "BREAKER";
        if (Kind == MapMarkerKind::BreakerActive)
            return "BREAKER ONLINE";
        if (Kind == MapMarkerKind::Exit)
            return "EXIT - NO POWER";
        if (Kind == MapMarkerKind::ExitPowered)
            return "POWERED EXIT";
        return "ENTITY";
    }
}

void Renderer::DrawMapPanelV2(
    const WorldData& World,
    int X,
    int Y,
    int PanelWidth,
    int PanelHeight,
    const glm::vec2& MapCenter,
    float PixelsPerMeter,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    const MapWaypointView& Waypoint,
    const std::vector<glm::vec2>& Route,
    bool Detailed,
    float Time
)
{
    const glm::vec3 Yellow{0.815f, 0.765f, 0.420f};
    const glm::vec3 Ink{0.105f, 0.095f, 0.045f};
    const glm::vec3 WallpaperLine{0.765f, 0.712f, 0.360f};
    const glm::vec3 WallpaperAccent{0.845f, 0.795f, 0.455f};
    const glm::vec3 Wall{0.145f, 0.132f, 0.063f};
    const glm::vec3 RouteOuter{0.055f, 0.060f, 0.070f};
    const glm::vec3 RouteColor{0.245f, 0.545f, 0.965f};

    auto ClipRect = [&](int RectX, int RectY, int RectWidth, int RectHeight, const glm::vec3& Color)
    {
        const int Left = std::max(RectX, X);
        const int Top = std::max(RectY, Y);
        const int Right = std::min(RectX + RectWidth, X + PanelWidth);
        const int Bottom = std::min(RectY + RectHeight, Y + PanelHeight);

        if (Right > Left && Bottom > Top)
            DrawRect(Left, Top, Right - Left, Bottom - Top, Color);
    };

    auto FillCircle = [&](int CenterX, int CenterY, int Radius, const glm::vec3& Color)
    {
        for (int OffsetY = -Radius; OffsetY <= Radius; ++OffsetY)
        {
            const float Inside =
                static_cast<float>(Radius * Radius - OffsetY * OffsetY);
            const int HalfWidth =
                Inside > 0.0f
                    ? static_cast<int>(std::sqrt(Inside))
                    : 0;

            ClipRect(
                CenterX - HalfWidth,
                CenterY + OffsetY,
                HalfWidth * 2 + 1,
                1,
                Color
            );
        }
    };

    auto Ring = [&](int CenterX, int CenterY, int Radius, int Thickness, const glm::vec3& Color)
    {
        FillCircle(CenterX, CenterY, Radius, Color);
        FillCircle(
            CenterX,
            CenterY,
            std::max(Radius - Thickness, 0),
            Yellow
        );
    };

    auto FillTriangle = [&](glm::ivec2 A, glm::ivec2 B, glm::ivec2 C, const glm::vec3& Color)
    {
        const int MinY = std::max(Y, std::min({A.y, B.y, C.y}));
        const int MaxY = std::min(Y + PanelHeight - 1, std::max({A.y, B.y, C.y}));

        for (int ScanY = MinY; ScanY <= MaxY; ++ScanY)
        {
            float Intersections[3]{};
            int Count = 0;

            auto AddIntersection = [&](glm::ivec2 P0, glm::ivec2 P1)
            {
                if (P0.y == P1.y)
                    return;

                const int EdgeMinY = std::min(P0.y, P1.y);
                const int EdgeMaxY = std::max(P0.y, P1.y);

                if (ScanY < EdgeMinY || ScanY >= EdgeMaxY)
                    return;

                const float T =
                    static_cast<float>(ScanY - P0.y) /
                    static_cast<float>(P1.y - P0.y);
                Intersections[Count++] =
                    static_cast<float>(P0.x) +
                    static_cast<float>(P1.x - P0.x) * T;
            };

            AddIntersection(A, B);
            AddIntersection(B, C);
            AddIntersection(C, A);

            if (Count < 2)
                continue;

            float Left = Intersections[0];
            float Right = Intersections[1];
            if (Left > Right)
                std::swap(Left, Right);

            const int StartX = static_cast<int>(std::floor(Left));
            const int EndX = static_cast<int>(std::ceil(Right));
            ClipRect(StartX, ScanY, std::max(EndX - StartX + 1, 1), 1, Color);
        }
    };

    auto Line = [&](glm::ivec2 A, glm::ivec2 B, int Thickness, const glm::vec3& Color)
    {
        const int DeltaX = B.x - A.x;
        const int DeltaY = B.y - A.y;
        const int Steps = std::max(std::abs(DeltaX), std::abs(DeltaY));

        if (Steps <= 0)
        {
            ClipRect(A.x, A.y, Thickness, Thickness, Color);
            return;
        }

        for (int I = 0; I <= Steps; ++I)
        {
            const float T = static_cast<float>(I) / static_cast<float>(Steps);
            const int DrawX = static_cast<int>(std::round(A.x + DeltaX * T));
            const int DrawY = static_cast<int>(std::round(A.y + DeltaY * T));
            ClipRect(
                DrawX - Thickness / 2,
                DrawY - Thickness / 2,
                Thickness,
                Thickness,
                Color
            );
        }
    };

    DrawRect(X, Y, PanelWidth, PanelHeight, Yellow);

    if (Detailed)
    {
        for (int StripeX = X + 28; StripeX < X + PanelWidth; StripeX += 56)
            ClipRect(StripeX, Y, 1, PanelHeight, WallpaperLine);

        for (int StripeX = X + 42; StripeX < X + PanelWidth; StripeX += 112)
            ClipRect(StripeX, Y, 1, PanelHeight, WallpaperAccent);

        for (int MotifY = Y + 34; MotifY < Y + PanelHeight; MotifY += 68)
        {
            for (int MotifX = X + 14; MotifX < X + PanelWidth; MotifX += 56)
                ClipRect(MotifX, MotifY, 8, 1, WallpaperLine);
        }
    }
    else
    {
        for (int StripeX = X + 24; StripeX < X + PanelWidth; StripeX += 48)
            ClipRect(StripeX, Y, 1, PanelHeight, WallpaperLine);

        for (int StripeX = X + 36; StripeX < X + PanelWidth; StripeX += 96)
            ClipRect(StripeX, Y, 1, PanelHeight, WallpaperAccent);
    }

    const float Scale = std::max(PixelsPerMeter, 0.1f);
    const float HalfCell = World.CellSize * 0.5f;

    glm::vec2 MapForward = PlayerForward;
    if (glm::length(MapForward) < 0.001f)
        MapForward = {0.0f, -1.0f};
    else
        MapForward = glm::normalize(MapForward);

    const glm::vec2 MapRight{-MapForward.y, MapForward.x};

    auto ToScreen = [&](float WorldX, float WorldZ)
    {
        const glm::vec2 Delta{
            WorldX - MapCenter.x,
            WorldZ - MapCenter.y
        };

        if (!Detailed)
        {
            return glm::ivec2{
                X + PanelWidth / 2 +
                    static_cast<int>(std::round(glm::dot(Delta, MapRight) * Scale)),
                Y + PanelHeight / 2 -
                    static_cast<int>(std::round(glm::dot(Delta, MapForward) * Scale))
            };
        }

        return glm::ivec2{
            X + PanelWidth / 2 +
                static_cast<int>(std::round(Delta.x * Scale)),
            Y + PanelHeight / 2 +
                static_cast<int>(std::round(Delta.y * Scale))
        };
    };

    const int WallThickness = 1;

    for (int LocalZ = 0; LocalZ < World.Rows; ++LocalZ)
    {
        for (int LocalX = 0; LocalX < World.Columns; ++LocalX)
        {
            const MazeCell& Cell = World.Cell(LocalX, LocalZ);
            const float CenterX = static_cast<float>(Cell.X) * World.CellSize;
            const float CenterZ = static_cast<float>(Cell.Z) * World.CellSize;

            if (!Detailed)
            {
                const glm::ivec2 NorthWest = ToScreen(CenterX - HalfCell, CenterZ - HalfCell);
                const glm::ivec2 NorthEast = ToScreen(CenterX + HalfCell, CenterZ - HalfCell);
                const glm::ivec2 SouthEast = ToScreen(CenterX + HalfCell, CenterZ + HalfCell);
                const glm::ivec2 SouthWest = ToScreen(CenterX - HalfCell, CenterZ + HalfCell);

                const int MinimumX = std::min({NorthWest.x, NorthEast.x, SouthEast.x, SouthWest.x});
                const int MaximumX = std::max({NorthWest.x, NorthEast.x, SouthEast.x, SouthWest.x});
                const int MinimumY = std::min({NorthWest.y, NorthEast.y, SouthEast.y, SouthWest.y});
                const int MaximumY = std::max({NorthWest.y, NorthEast.y, SouthEast.y, SouthWest.y});

                if (
                    MaximumX < X ||
                    MaximumY < Y ||
                    MinimumX > X + PanelWidth ||
                    MinimumY > Y + PanelHeight
                )
                {
                    continue;
                }

                if (Cell.Walls[0])
                    Line(NorthWest, NorthEast, WallThickness, Wall);
                if (Cell.Walls[1])
                    Line(NorthEast, SouthEast, WallThickness, Wall);
                if (Cell.Walls[2])
                    Line(SouthWest, SouthEast, WallThickness, Wall);
                if (Cell.Walls[3])
                    Line(NorthWest, SouthWest, WallThickness, Wall);

                continue;
            }

            const glm::ivec2 TopLeft = ToScreen(CenterX - HalfCell, CenterZ - HalfCell);
            const glm::ivec2 BottomRight = ToScreen(CenterX + HalfCell, CenterZ + HalfCell);

            if (
                BottomRight.x < X ||
                BottomRight.y < Y ||
                TopLeft.x > X + PanelWidth ||
                TopLeft.y > Y + PanelHeight
            )
            {
                continue;
            }

            const int CellWidth = std::max(BottomRight.x - TopLeft.x, 1);
            const int CellHeight = std::max(BottomRight.y - TopLeft.y, 1);

            if (Cell.Walls[0])
                ClipRect(TopLeft.x, TopLeft.y, CellWidth, WallThickness, Wall);
            if (Cell.Walls[1])
                ClipRect(BottomRight.x - WallThickness, TopLeft.y, WallThickness, CellHeight, Wall);
            if (Cell.Walls[2])
                ClipRect(TopLeft.x, BottomRight.y - WallThickness, CellWidth, WallThickness, Wall);
            if (Cell.Walls[3])
                ClipRect(TopLeft.x, TopLeft.y, WallThickness, CellHeight, Wall);
        }
    }

    if (Route.size() >= 2)
    {
        for (std::size_t I = 1; I < Route.size(); ++I)
        {
            const glm::ivec2 A = ToScreen(Route[I - 1].x, Route[I - 1].y);
            const glm::ivec2 B = ToScreen(Route[I].x, Route[I].y);
            Line(A, B, Detailed ? 6 : 3, RouteOuter);
            Line(A, B, Detailed ? 3 : 1, RouteColor);
        }
    }

    float HoverDistance = std::numeric_limits<float>::max();
    std::string HoverLabel;
    glm::ivec2 HoverScreen{0};

    for (const MapMarker& Marker : Markers)
    {
        const glm::ivec2 Position = ToScreen(Marker.Position.x, Marker.Position.y);

        if (Marker.Kind == MapMarkerKind::Breaker || Marker.Kind == MapMarkerKind::BreakerActive)
        {
            const bool Active = Marker.Kind == MapMarkerKind::BreakerActive;
            const glm::vec3 Glow = Active
                ? glm::vec3{0.16f, 0.92f, 0.34f}
                : glm::vec3{1.0f, 0.13f, 0.08f};

            if (!Detailed)
            {
                FillCircle(Position.x, Position.y, 3, Ink);
                FillCircle(Position.x, Position.y, 2, Glow);
            }
            else
            {
                const int Radius = 8;
                FillCircle(Position.x, Position.y, Radius + 3, Ink);
                FillCircle(Position.x, Position.y, Radius + 1, Glow);
                FillCircle(Position.x, Position.y, Radius - 2, Yellow);
                Line(
                    {Position.x - 4, Position.y + 4},
                    {Position.x + 4, Position.y - 4},
                    2,
                    Ink
                );
                FillCircle(Position.x + 4, Position.y - 4, 2, Ink);
                ClipRect(Position.x - 6, Position.y + 3, 5, 3, Ink);
            }
        }
        else if (Marker.Kind == MapMarkerKind::Exit || Marker.Kind == MapMarkerKind::ExitPowered)
        {
            const glm::vec3 Color =
                Marker.Kind == MapMarkerKind::ExitPowered
                    ? glm::vec3{0.16f, 0.95f, 0.42f}
                    : glm::vec3{0.96f, 0.94f, 0.80f};

            if (!Detailed)
            {
                ClipRect(Position.x - 3, Position.y - 3, 7, 7, Ink);
                ClipRect(Position.x - 2, Position.y - 2, 5, 5, Color);
                ClipRect(Position.x, Position.y - 1, 1, 3, Ink);
            }
            else
            {
                const int Radius = 8;
                FillCircle(Position.x, Position.y, Radius + 2, Ink);
                FillCircle(Position.x, Position.y, Radius, Color);
                ClipRect(Position.x - 1, Position.y - 3, 2, 6, Ink);
                ClipRect(Position.x + 2, Position.y, 2, 2, Ink);
            }
        }
        else
        {
            const bool RedPhase = std::fmod(Time * 5.0f, 1.0f) < 0.58f;
            const glm::vec3 Threat = RedPhase
                ? glm::vec3{1.0f, 0.035f, 0.02f}
                : glm::vec3{0.50f, 0.015f, 0.012f};

            if (!Detailed)
            {
                ClipRect(Position.x - 3, Position.y - 3, 7, 7, Ink);
                ClipRect(Position.x - 2, Position.y - 2, 5, 5, Threat);
            }
            else
            {
                const int Radius = 9;
                const glm::ivec2 Top{Position.x, Position.y - Radius};
                const glm::ivec2 LeftPoint{Position.x - Radius, Position.y + Radius - 2};
                const glm::ivec2 RightPoint{Position.x + Radius, Position.y + Radius - 2};
                FillTriangle(
                    {Top.x + 1, Top.y + 2},
                    {LeftPoint.x + 1, LeftPoint.y + 2},
                    {RightPoint.x + 1, RightPoint.y + 2},
                    Ink
                );
                FillTriangle(Top, LeftPoint, RightPoint, Threat);
                ClipRect(Position.x - 1, Position.y - 2, 2, 6, Yellow);
                FillCircle(Position.x, Position.y + 6, 1, Yellow);
            }
        }

        if (Detailed)
        {
            const float DeltaX = static_cast<float>(Position.x) - MenuPointerX;
            const float DeltaY = static_cast<float>(Position.y) - MenuPointerY;
            const float Distance = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);

            if (Distance < 18.0f && Distance < HoverDistance)
            {
                HoverDistance = Distance;
                const float Meters = glm::distance(PlayerPosition, Marker.Position);
                std::ostringstream Label;
                Label << MarkerName(Marker.Kind) << "   " << std::fixed << std::setprecision(0) << Meters << " M";
                HoverLabel = Label.str();
                HoverScreen = Position;
            }
        }
    }

    if (Waypoint.Active)
    {
        const glm::ivec2 Position = ToScreen(Waypoint.Position.x, Waypoint.Position.y);
        if (!Detailed)
        {
            Ring(Position.x, Position.y, 4, 1, Ink);
            ClipRect(Position.x - 1, Position.y - 1, 3, 3, RouteColor);
        }
        else
        {
            const int Pulse = 9 + static_cast<int>((std::sin(Time * 5.0f) + 1.0f) * 2.0f);
            Ring(Position.x, Position.y, Pulse + 3, 2, Ink);
            Ring(Position.x, Position.y, Pulse, 2, RouteColor);
            Line({Position.x - 5, Position.y}, {Position.x + 5, Position.y}, 2, RouteColor);
            Line({Position.x, Position.y - 5}, {Position.x, Position.y + 5}, 2, RouteColor);
        }

        if (Detailed)
        {
            const float DeltaX = static_cast<float>(Position.x) - MenuPointerX;
            const float DeltaY = static_cast<float>(Position.y) - MenuPointerY;
            const float Distance = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);

            if (Distance < 20.0f && Distance < HoverDistance)
            {
                std::ostringstream Label;
                Label << "WAYPOINT   " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";
                HoverLabel = Label.str();
                HoverScreen = Position;
            }
        }
    }

    const glm::ivec2 PlayerScreen = ToScreen(PlayerPosition.x, PlayerPosition.y);
    glm::vec2 Direction = Detailed ? PlayerForward : glm::vec2{0.0f, -1.0f};

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const glm::vec2 Right{-Direction.y, Direction.x};
    const int ArrowLength = Detailed ? 14 : 5;
    const int ArrowWidth = Detailed ? 8 : 3;
    const float TailLength = Detailed ? 5.0f : 1.0f;
    const glm::ivec2 Tip{
        PlayerScreen.x + static_cast<int>(std::round(Direction.x * ArrowLength)),
        PlayerScreen.y + static_cast<int>(std::round(Direction.y * ArrowLength))
    };
    const glm::ivec2 Left{
        PlayerScreen.x - static_cast<int>(std::round(Direction.x * TailLength)) + static_cast<int>(std::round(Right.x * ArrowWidth)),
        PlayerScreen.y - static_cast<int>(std::round(Direction.y * TailLength)) + static_cast<int>(std::round(Right.y * ArrowWidth))
    };
    const glm::ivec2 RightPoint{
        PlayerScreen.x - static_cast<int>(std::round(Direction.x * TailLength)) - static_cast<int>(std::round(Right.x * ArrowWidth)),
        PlayerScreen.y - static_cast<int>(std::round(Direction.y * TailLength)) - static_cast<int>(std::round(Right.y * ArrowWidth))
    };

    if (Detailed)
    {
        FillTriangle(
            {Tip.x + 2, Tip.y + 2},
            {Left.x + 2, Left.y + 2},
            {RightPoint.x + 2, RightPoint.y + 2},
            Ink
        );
    }
    FillTriangle(Tip, Left, RightPoint, {0.98f, 0.97f, 0.86f});
    FillCircle(PlayerScreen.x, PlayerScreen.y, Detailed ? 3 : 1, RouteColor);

    if (Detailed && !HoverLabel.empty() && GameplayTextRenderer.IsReady())
    {
        const int FontSize = 11;
        const int TextWidth = GameplayTextRenderer.Measure(HoverLabel, FontSize, 800, 0.05f);
        int LabelX = HoverScreen.x + 16;
        int LabelY = HoverScreen.y - 28;

        if (LabelX + TextWidth + 20 > X + PanelWidth)
            LabelX = HoverScreen.x - TextWidth - 28;
        if (LabelY < Y + 8)
            LabelY = HoverScreen.y + 18;

        DrawRect(LabelX - 8, LabelY - 6, TextWidth + 16, FontSize + 14, Ink);
        GameplayTextRenderer.Draw(
            HoverLabel,
            LabelX,
            LabelY,
            FontSize,
            800,
            0.05f,
            {0.98f, 0.95f, 0.74f},
            1.0f,
            false
        );
    }

    const glm::vec3 Frame{0.24f, 0.22f, 0.10f};
    const int FrameThickness = Detailed ? 1 : 2;
    DrawRect(X, Y, PanelWidth, FrameThickness, Frame);
    DrawRect(X, Y + PanelHeight - FrameThickness, PanelWidth, FrameThickness, Frame);
    DrawRect(X, Y, FrameThickness, PanelHeight, Frame);
    DrawRect(X + PanelWidth - FrameThickness, Y, FrameThickness, PanelHeight, Frame);
}

void Renderer::DrawMiniMapV2(
    const WorldData& World,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    const MapWaypointView& Waypoint,
    const std::vector<glm::vec2>& Route,
    float ThreatDistance,
    float Time
)
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    const bool Compact = Width < 900 || Height < 620;
    const int MapWidth = std::clamp(
        static_cast<int>(Width * (Compact ? 0.29f : 0.19f)),
        Compact ? 220 : 280,
        Compact ? 280 : 330
    );
    const int MapHeight = std::clamp(
        static_cast<int>(Height * (Compact ? 0.22f : 0.205f)),
        Compact ? 148 : 172,
        Compact ? 188 : 202
    );
    const int Margin = std::clamp(static_cast<int>(Width * 0.015f), 16, 26);
    const int MapX = Margin;
    const int MapY = static_cast<int>(Height) - MapHeight - Margin;
    const int HeaderHeight = 30;
    const int FooterHeight = Waypoint.Active ? 25 : 0;
    const int FrameThickness = 2;
    const int InnerPadding = 6;

    const int InnerX = MapX + FrameThickness + InnerPadding;
    const int InnerY = MapY + HeaderHeight + InnerPadding;
    const int InnerWidth = std::max(
        MapWidth - (FrameThickness + InnerPadding) * 2,
        1
    );
    const int InnerHeight = std::max(
        MapHeight - HeaderHeight - FooterHeight - FrameThickness - InnerPadding * 2,
        1
    );

    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 WarmText{0.97f, 0.93f, 0.66f};
    const glm::vec3 PanelFill{0.805f, 0.755f, 0.405f};
    const glm::vec3 Frame{0.075f, 0.068f, 0.032f};
    const glm::vec3 Shadow{0.055f, 0.050f, 0.026f};
    const glm::vec3 HeaderIdle{0.115f, 0.105f, 0.052f};
    const glm::vec3 HeaderAlert{0.42f, 0.018f, 0.012f};
    const glm::vec3 RouteAccent{0.245f, 0.545f, 0.965f};
    const bool ThreatNear = std::isfinite(ThreatDistance) && ThreatDistance < 36.0f;
    const bool RedPhase = ThreatNear && std::fmod(Time * 7.0f, 1.0f) < 0.5f;
    const glm::vec3 HeaderFill = RedPhase ? HeaderAlert : HeaderIdle;

    DrawRect(MapX + 4, MapY + 4, MapWidth, MapHeight, Shadow);
    DrawRect(MapX, MapY, MapWidth, MapHeight, Frame);
    DrawRect(
        MapX + FrameThickness,
        MapY + FrameThickness,
        MapWidth - FrameThickness * 2,
        MapHeight - FrameThickness * 2,
        PanelFill
    );

    DrawMapPanelV2(
        World,
        InnerX,
        InnerY,
        InnerWidth,
        InnerHeight,
        PlayerPosition,
        Compact ? 2.45f : 2.70f,
        PlayerPosition,
        PlayerForward,
        Markers,
        Waypoint,
        Route,
        false,
        Time
    );

    DrawRect(
        MapX + FrameThickness,
        MapY + FrameThickness,
        MapWidth - FrameThickness * 2,
        HeaderHeight - FrameThickness,
        HeaderFill
    );
    DrawRect(
        MapX + FrameThickness,
        MapY + HeaderHeight - 2,
        MapWidth - FrameThickness * 2,
        2,
        ThreatNear ? glm::vec3{0.95f, 0.05f, 0.025f} : RouteAccent
    );

    if (Waypoint.Active)
    {
        const int FooterY = MapY + MapHeight - FooterHeight - FrameThickness;
        DrawRect(
            MapX + FrameThickness,
            FooterY,
            MapWidth - FrameThickness * 2,
            FooterHeight,
            HeaderIdle
        );
        DrawRect(
            MapX + FrameThickness,
            FooterY,
            MapWidth - FrameThickness * 2,
            2,
            RouteAccent
        );
    }

    if (ThreatNear)
    {
        const glm::vec3 Alert = RedPhase
            ? glm::vec3{0.98f, 0.025f, 0.015f}
            : glm::vec3{0.04f, 0.02f, 0.012f};
        const int Border = RedPhase ? 4 : 3;
        DrawRect(MapX - Border, MapY - Border, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY + MapHeight, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY, Border, MapHeight, Alert);
        DrawRect(MapX + MapWidth, MapY, Border, MapHeight, Alert);
    }

    if (GameplayTextRenderer.IsReady())
    {
        GameplayTextRenderer.Resize(Width, Height);

        GameplayTextRenderer.Draw(
            ThreatNear ? "THREAT" : "MAP",
            MapX + 11,
            MapY + 7,
            11,
            850,
            0.08f,
            ThreatNear ? glm::vec3{1.0f, 0.76f, 0.62f} : WarmText,
            1.0f,
            true
        );

        GameplayTextRenderer.Draw(
            "LEVEL 0",
            MapX + 61,
            MapY + 8,
            9,
            700,
            0.08f,
            WarmText,
            0.82f,
            true
        );

        const std::string ExpandHint = "M  EXPAND";
        const int ExpandWidth = GameplayTextRenderer.Measure(
            ExpandHint,
            9,
            750,
            0.05f
        );
        GameplayTextRenderer.Draw(
            ExpandHint,
            MapX + MapWidth - 11 - ExpandWidth,
            MapY + 8,
            9,
            750,
            0.05f,
            WarmText,
            0.86f,
            true
        );

        if (Waypoint.Active)
        {
            std::ostringstream Distance;
            Distance
                << "DESTINATION  "
                << std::fixed
                << std::setprecision(0)
                << Waypoint.DistanceMeters
                << " M";

            GameplayTextRenderer.Draw(
                Distance.str(),
                MapX + 11,
                MapY + MapHeight - FooterHeight + 6,
                9,
                800,
                0.05f,
                WarmText,
                1.0f,
                true
            );
        }
    }

    DrawRect(MapX, MapY, MapWidth, FrameThickness, Frame);
    DrawRect(MapX, MapY + MapHeight - FrameThickness, MapWidth, FrameThickness, Frame);
    DrawRect(MapX, MapY, FrameThickness, MapHeight, Frame);
    DrawRect(MapX + MapWidth - FrameThickness, MapY, FrameThickness, MapHeight, Frame);
}

void Renderer::DrawFullMapV2(
    const WorldData& World,
    const glm::vec2& MapCenter,
    float Zoom,
    const glm::vec2& PlayerPosition,
    const glm::vec2& PlayerForward,
    const std::vector<MapMarker>& Markers,
    const MapWaypointView& Waypoint,
    const std::vector<glm::vec2>& Route,
    int BreakersActive,
    int BreakersRequired,
    float Time
)
{
    if (!GameplayTextRenderer.IsReady())
        GameplayTextRenderer.Initialize();

    GameplayTextRenderer.Resize(Width, Height);
    glDisable(GL_DEPTH_TEST);

    const glm::vec3 Yellow{0.815f, 0.765f, 0.420f};
    const glm::vec3 Ink{0.105f, 0.095f, 0.045f};
    const glm::vec3 Muted{0.34f, 0.31f, 0.16f};
    const glm::vec3 ButtonIdle{0.770f, 0.716f, 0.360f};
    const glm::vec3 ButtonHover{0.145f, 0.135f, 0.072f};
    const glm::vec3 ButtonHoverText{0.96f, 0.93f, 0.73f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Yellow);

    for (int X = 32; X < static_cast<int>(Width); X += 64)
        DrawRect(X, 0, 1, static_cast<int>(Height), {0.775f, 0.724f, 0.370f});

    const bool Compact = Width < 930 || Height < 650;
    const int Margin = Compact ? 14 : 28;
    const int ButtonHeight = Compact ? 34 : 40;
    const int HeaderHeight = Compact ? 94 : 108;
    const int ButtonY = 16;
    const int Gap = Compact ? 7 : 10;

    const int BackWidth = Compact ? 112 : 142;
    const int RandomWidth = Compact ? 148 : 190;
    const int ClearWidth = Compact ? 116 : 148;

    FullMapBackRect = {Margin, ButtonY, BackWidth, ButtonHeight};
    FullMapClearRect = {
        static_cast<int>(Width) - Margin - ClearWidth,
        ButtonY,
        ClearWidth,
        ButtonHeight
    };
    FullMapRandomRect = {
        FullMapClearRect.X - Gap - RandomWidth,
        ButtonY,
        RandomWidth,
        ButtonHeight
    };

    auto DrawButton = [&](const UiRect& Rect, const std::string& Label, float Hover)
    {
        const bool Hovered = Hover > 0.05f;
        const glm::vec3 Fill = Hovered ? ButtonHover : ButtonIdle;
        const glm::vec3 TextColor = Hovered ? ButtonHoverText : Ink;

        DrawRect(Rect.X, Rect.Y, Rect.Width, Rect.Height, Fill);
        DrawRect(
            Rect.X,
            Rect.Y + Rect.Height - 2,
            Rect.Width,
            2,
            Hovered ? glm::vec3{0.245f, 0.545f, 0.965f} : glm::vec3{0.43f, 0.39f, 0.18f}
        );

        GameplayTextRenderer.Draw(
            Label,
            Rect.X + 14,
            Rect.Y + (Rect.Height - 11) / 2,
            11,
            780,
            0.035f,
            TextColor,
            1.0f,
            false
        );
    };

    DrawButton(FullMapBackRect, "\xE2\x86\x90  BACK", FullMapBackHover);
    DrawButton(FullMapRandomRect, "RANDOM DESTINATION", FullMapRandomHover);
    DrawButton(FullMapClearRect, "CLEAR MARKER", FullMapClearHover);

    const int TitleY = ButtonY + ButtonHeight + (Compact ? 10 : 12);
    GameplayTextRenderer.Draw(
        "LEVEL 0 / MAP",
        Margin,
        TitleY,
        Compact ? 20 : 25,
        900,
        0.04f,
        Ink,
        1.0f,
        false
    );

    std::ostringstream Status;
    Status << "POWER " << BreakersActive << "/" << BreakersRequired;

    if (Waypoint.Active)
        Status << "     WAYPOINT " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";

    const int StatusWidth = GameplayTextRenderer.Measure(Status.str(), 10, 800, 0.08f);
    GameplayTextRenderer.Draw(
        Status.str(),
        std::max(Margin, static_cast<int>(Width) - Margin - StatusWidth),
        TitleY + 5,
        10,
        800,
        0.08f,
        Muted,
        1.0f,
        false
    );

    const int FooterHeight = Compact ? 52 : 60;
    FullMapRect = {
        Margin,
        HeaderHeight,
        std::max(static_cast<int>(Width) - Margin * 2, 1),
        std::max(static_cast<int>(Height) - HeaderHeight - FooterHeight, 1)
    };

    DrawMapPanelV2(
        World,
        FullMapRect.X,
        FullMapRect.Y,
        FullMapRect.Width,
        FullMapRect.Height,
        MapCenter,
        FullMapScale(Zoom),
        PlayerPosition,
        PlayerForward,
        Markers,
        Waypoint,
        Route,
        true,
        Time
    );

    std::ostringstream ZoomText;
    ZoomText << "ZOOM " << std::fixed << std::setprecision(0) << std::clamp(Zoom, 0.45f, 4.0f) * 100.0f << "%";

    const int FooterY = static_cast<int>(Height) - FooterHeight + 11;
    GameplayTextRenderer.Draw(
        "ESC BACK   /   CLICK WAYPOINT   /   DRAG PAN   /   WHEEL ZOOM   /   R RECENTER   /   G RANDOM",
        Margin,
        FooterY,
        Compact ? 8 : 9,
        720,
        0.045f,
        Muted,
        1.0f,
        false
    );

    GameplayTextRenderer.Draw(
        "HOVER ICONS FOR LABELS   |   WAYPOINT CLEARS WHEN REACHED",
        Margin,
        FooterY + 17,
        Compact ? 8 : 9,
        650,
        0.04f,
        Muted,
        0.9f,
        false
    );

    const int ZoomWidth = GameplayTextRenderer.Measure(ZoomText.str(), 9, 800, 0.06f);
    GameplayTextRenderer.Draw(
        ZoomText.str(),
        static_cast<int>(Width) - Margin - ZoomWidth,
        FooterY + 17,
        9,
        800,
        0.06f,
        Ink,
        1.0f,
        false
    );

    glEnable(GL_DEPTH_TEST);
}

MapUiAction Renderer::HitTestFullMap() const
{
    if (FullMapBackRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::Back;
    if (FullMapRandomRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::RandomDestination;
    if (FullMapClearRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::ClearWaypoint;
    return MapUiAction::None;
}

bool Renderer::PointerInsideFullMap() const
{
    return FullMapRect.Contains(MenuPointerX, MenuPointerY);
}

glm::vec2 Renderer::FullMapPointerToWorld(
    const glm::vec2& MapCenter,
    float Zoom
) const
{
    const float Scale = FullMapScale(Zoom);
    return {
        MapCenter.x +
            (MenuPointerX - static_cast<float>(FullMapRect.X + FullMapRect.Width / 2)) / Scale,
        MapCenter.y +
            (MenuPointerY - static_cast<float>(FullMapRect.Y + FullMapRect.Height / 2)) / Scale
    };
}
