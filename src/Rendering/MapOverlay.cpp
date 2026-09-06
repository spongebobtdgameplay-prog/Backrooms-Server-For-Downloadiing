#include "Renderer.h"
#include "GeneratedWebUi.h"

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
    auto CssColor = [](const GeneratedWebUi::CssColor& Color)
    {
        return glm::vec3{Color.R, Color.G, Color.B};
    };

    const glm::vec3 Background = CssColor(GeneratedWebUi::MapBackgroundColor);
    const glm::vec3 Grid = CssColor(GeneratedWebUi::MapGridColor);
    const glm::vec3 Wall = CssColor(GeneratedWebUi::MapWallColor);
    const glm::vec3 Frame = CssColor(GeneratedWebUi::MapFrameColor);
    const glm::vec3 RouteOuter = CssColor(GeneratedWebUi::MapRouteOuterColor);
    const glm::vec3 RouteColor = Detailed
        ? CssColor(GeneratedWebUi::FullMapRouteColor)
        : CssColor(GeneratedWebUi::MiniMapRouteColor);
    const glm::vec3 PlayerColor = Detailed
        ? CssColor(GeneratedWebUi::FullMapPlayerColor)
        : CssColor(GeneratedWebUi::MiniMapPlayerColor);
    const glm::vec3 WaypointColor = Detailed
        ? CssColor(GeneratedWebUi::FullMapWaypointColor)
        : CssColor(GeneratedWebUi::MiniMapWaypointColor);

    DrawRect(X, Y, PanelWidth, PanelHeight, Background);

    if (Detailed)
    {
        const int MajorGrid = 128;
        for (int GridX = X + MajorGrid; GridX < X + PanelWidth; GridX += MajorGrid)
            DrawRect(GridX, Y, 1, PanelHeight, Grid);
        for (int GridY = Y + MajorGrid; GridY < Y + PanelHeight; GridY += MajorGrid)
            DrawRect(X, GridY, PanelWidth, 1, Grid);
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
        const glm::vec2 Delta{WorldX - MapCenter.x, WorldZ - MapCenter.y};

        if (!Detailed)
        {
            return glm::vec2{
                static_cast<float>(X) + static_cast<float>(PanelWidth) * 0.5f +
                    glm::dot(Delta, MapRight) * Scale,
                static_cast<float>(Y) + static_cast<float>(PanelHeight) * 0.5f -
                    glm::dot(Delta, MapForward) * Scale
            };
        }

        return glm::vec2{
            static_cast<float>(X) + static_cast<float>(PanelWidth) * 0.5f + Delta.x * Scale,
            static_cast<float>(Y) + static_cast<float>(PanelHeight) * 0.5f + Delta.y * Scale
        };
    };

    auto AppendLine = [](
        std::vector<glm::vec2>& Vertices,
        glm::vec2 A,
        glm::vec2 B,
        float Thickness
    )
    {
        const glm::vec2 Delta = B - A;
        const float Length = glm::length(Delta);
        if (Length <= 0.001f || Thickness <= 0.0f)
            return;

        const glm::vec2 Normal =
            glm::vec2{-Delta.y, Delta.x} / Length * (Thickness * 0.5f);

        const glm::vec2 A0 = A + Normal;
        const glm::vec2 A1 = A - Normal;
        const glm::vec2 B0 = B + Normal;
        const glm::vec2 B1 = B - Normal;

        Vertices.insert(Vertices.end(), {A0, B0, B1, A0, B1, A1});
    };

    auto DrawTriangle = [&](glm::vec2 A, glm::vec2 B, glm::vec2 C, const glm::vec3& Color)
    {
        DrawUiTriangles({A, B, C}, Color);
    };

    auto DrawCircle = [&](glm::vec2 Center, float Radius, const glm::vec3& Color)
    {
        constexpr int Segments = 24;
        std::vector<glm::vec2> Vertices;
        Vertices.reserve(Segments * 3);
        for (int I = 0; I < Segments; ++I)
        {
            const float A0 = static_cast<float>(I) / static_cast<float>(Segments) * 6.28318530718f;
            const float A1 = static_cast<float>(I + 1) / static_cast<float>(Segments) * 6.28318530718f;
            Vertices.push_back(Center);
            Vertices.push_back(Center + glm::vec2{std::cos(A0), std::sin(A0)} * Radius);
            Vertices.push_back(Center + glm::vec2{std::cos(A1), std::sin(A1)} * Radius);
        }
        DrawUiTriangles(Vertices, Color);
    };

    auto DrawDiamond = [&](glm::vec2 Center, float Radius, const glm::vec3& Color)
    {
        DrawUiTriangles(
            {
                {Center.x, Center.y - Radius},
                {Center.x + Radius, Center.y},
                {Center.x, Center.y + Radius},
                {Center.x, Center.y - Radius},
                {Center.x, Center.y + Radius},
                {Center.x - Radius, Center.y}
            },
            Color
        );
    };

    auto DrawArrow = [&](glm::vec2 Center, glm::vec2 Direction, float Length, float ArrowWidth, const glm::vec3& Outer, const glm::vec3& Inner)
    {
        if (glm::length(Direction) < 0.001f)
            Direction = {0.0f, -1.0f};
        else
            Direction = glm::normalize(Direction);

        const glm::vec2 Right{-Direction.y, Direction.x};

        auto TriangleFor = [&](float Extra)
        {
            const float L = Length + Extra;
            const float W = ArrowWidth + Extra * 0.65f;
            const glm::vec2 Tip = Center + Direction * L * 0.58f;
            const glm::vec2 Tail = Center - Direction * L * 0.42f;
            return std::array<glm::vec2, 3>{
                Tip,
                Tail + Right * W * 0.5f,
                Tail - Right * W * 0.5f
            };
        };

        const auto OuterTriangle = TriangleFor(2.4f);
        DrawTriangle(OuterTriangle[0], OuterTriangle[1], OuterTriangle[2], Outer);
        const auto InnerTriangle = TriangleFor(0.0f);
        DrawTriangle(InnerTriangle[0], InnerTriangle[1], InnerTriangle[2], Inner);
    };

    auto ClampCompactMarker = [&](glm::vec2 Raw, float Inset, bool& Offscreen)
    {
        const float Left = static_cast<float>(X) + Inset;
        const float Right = static_cast<float>(X + PanelWidth) - Inset;
        const float Top = static_cast<float>(Y) + Inset;
        const float Bottom = static_cast<float>(Y + PanelHeight) - Inset;

        Offscreen =
            Raw.x < Left || Raw.x > Right ||
            Raw.y < Top || Raw.y > Bottom;

        if (Detailed)
            return Raw;

        return glm::vec2{
            std::clamp(Raw.x, Left, Right),
            std::clamp(Raw.y, Top, Bottom)
        };
    };

    std::vector<glm::vec2> WallVertices;
    WallVertices.reserve(static_cast<std::size_t>(World.Columns * World.Rows) * 12u);

    const float WallThickness = static_cast<float>(
        Detailed ? 1 : std::max(GeneratedWebUi::MiniMapWallThickness, 1)
    );

    for (int LocalZ = 0; LocalZ < World.Rows; ++LocalZ)
    {
        for (int LocalX = 0; LocalX < World.Columns; ++LocalX)
        {
            const MazeCell& Cell = World.Cell(LocalX, LocalZ);
            const float CenterX = static_cast<float>(Cell.X) * World.CellSize;
            const float CenterZ = static_cast<float>(Cell.Z) * World.CellSize;

            const glm::vec2 NorthWest = ToScreen(CenterX - HalfCell, CenterZ - HalfCell);
            const glm::vec2 NorthEast = ToScreen(CenterX + HalfCell, CenterZ - HalfCell);
            const glm::vec2 SouthEast = ToScreen(CenterX + HalfCell, CenterZ + HalfCell);
            const glm::vec2 SouthWest = ToScreen(CenterX - HalfCell, CenterZ + HalfCell);

            const float MinimumX = std::min({NorthWest.x, NorthEast.x, SouthEast.x, SouthWest.x});
            const float MaximumX = std::max({NorthWest.x, NorthEast.x, SouthEast.x, SouthWest.x});
            const float MinimumY = std::min({NorthWest.y, NorthEast.y, SouthEast.y, SouthWest.y});
            const float MaximumY = std::max({NorthWest.y, NorthEast.y, SouthEast.y, SouthWest.y});

            if (
                MaximumX < static_cast<float>(X) - 3.0f ||
                MaximumY < static_cast<float>(Y) - 3.0f ||
                MinimumX > static_cast<float>(X + PanelWidth) + 3.0f ||
                MinimumY > static_cast<float>(Y + PanelHeight) + 3.0f
            )
            {
                continue;
            }

            if (Cell.Walls[0])
                AppendLine(WallVertices, NorthWest, NorthEast, WallThickness);
            if (Cell.Walls[3])
                AppendLine(WallVertices, NorthWest, SouthWest, WallThickness);
            if (LocalX == World.Columns - 1 && Cell.Walls[1])
                AppendLine(WallVertices, NorthEast, SouthEast, WallThickness);
            if (LocalZ == World.Rows - 1 && Cell.Walls[2])
                AppendLine(WallVertices, SouthWest, SouthEast, WallThickness);
        }
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(
        X,
        static_cast<int>(Height) - (Y + PanelHeight),
        PanelWidth,
        PanelHeight
    );

    DrawUiTriangles(WallVertices, Wall);

    std::vector<glm::vec2> SimplifiedRoute;
    SimplifiedRoute.reserve(Route.size());

    for (const glm::vec2& Point : Route)
    {
        if (SimplifiedRoute.size() < 2)
        {
            SimplifiedRoute.push_back(Point);
            continue;
        }

        const glm::vec2 A = SimplifiedRoute[SimplifiedRoute.size() - 2];
        const glm::vec2 B = SimplifiedRoute.back();
        const glm::vec2 AB = B - A;
        const glm::vec2 BC = Point - B;
        const float Cross = AB.x * BC.y - AB.y * BC.x;
        const float Dot = glm::dot(AB, BC);

        if (std::abs(Cross) < 0.001f && Dot >= 0.0f)
            SimplifiedRoute.back() = Point;
        else
            SimplifiedRoute.push_back(Point);
    }

    const int RouteThickness = Detailed
        ? std::max(GeneratedWebUi::FullMapRouteThickness, 1)
        : std::max(GeneratedWebUi::MiniMapRouteThickness, 1);

    if (SimplifiedRoute.size() >= 2)
    {
        std::vector<glm::vec2> RouteOuterVertices;
        std::vector<glm::vec2> RouteVertices;
        RouteOuterVertices.reserve((SimplifiedRoute.size() - 1) * 6);
        RouteVertices.reserve((SimplifiedRoute.size() - 1) * 6);

        for (std::size_t I = 1; I < SimplifiedRoute.size(); ++I)
        {
            const glm::vec2 A = ToScreen(SimplifiedRoute[I - 1].x, SimplifiedRoute[I - 1].y);
            const glm::vec2 B = ToScreen(SimplifiedRoute[I].x, SimplifiedRoute[I].y);
            AppendLine(
                RouteOuterVertices,
                A,
                B,
                static_cast<float>(std::max(GeneratedWebUi::MapRouteOuterThickness, RouteThickness + 1))
            );
            AppendLine(RouteVertices, A, B, static_cast<float>(RouteThickness));
        }

        DrawUiTriangles(RouteOuterVertices, RouteOuter);
        DrawUiTriangles(RouteVertices, RouteColor);
    }

    float HoverDistance = std::numeric_limits<float>::max();
    std::string HoverLabel;
    glm::vec2 HoverScreen{0.0f};

    const glm::vec2 PanelCenter{
        static_cast<float>(X) + static_cast<float>(PanelWidth) * 0.5f,
        static_cast<float>(Y) + static_cast<float>(PanelHeight) * 0.5f
    };

    for (const MapMarker& Marker : Markers)
    {
        const glm::vec2 RawPosition = ToScreen(Marker.Position.x, Marker.Position.y);
        bool Offscreen = false;
        const glm::vec2 Position = ClampCompactMarker(RawPosition, 10.0f, Offscreen);

        if (Marker.Kind == MapMarkerKind::Breaker || Marker.Kind == MapMarkerKind::BreakerActive)
        {
            const glm::vec3 Color = Marker.Kind == MapMarkerKind::BreakerActive
                ? CssColor(GeneratedWebUi::MiniMapBreakerActiveColor)
                : CssColor(GeneratedWebUi::MiniMapBreakerColor);
            const float Radius = Detailed ? 6.0f : 4.4f;
            DrawCircle(Position, Radius + 2.0f, Frame);
            DrawCircle(Position, Radius, Color);
            DrawCircle(Position, Radius * 0.42f, Background);
        }
        else if (Marker.Kind == MapMarkerKind::Exit || Marker.Kind == MapMarkerKind::ExitPowered)
        {
            const glm::vec3 Color = Marker.Kind == MapMarkerKind::ExitPowered
                ? CssColor(GeneratedWebUi::MiniMapExitPoweredColor)
                : CssColor(GeneratedWebUi::MiniMapExitColor);
            const float Radius = Detailed ? 7.0f : 5.0f;
            DrawDiamond(Position, Radius + 2.0f, Frame);
            DrawDiamond(Position, Radius, Color);
            DrawDiamond(Position, Radius * 0.42f, Background);
        }
        else
        {
            const glm::vec3 Color = CssColor(GeneratedWebUi::MiniMapThreatColor);
            glm::vec2 MarkerDirection = RawPosition - PanelCenter;
            if (glm::length(MarkerDirection) < 0.001f)
                MarkerDirection = {0.0f, -1.0f};
            const float Size = Detailed ? 9.0f : 6.0f;
            DrawArrow(Position, MarkerDirection, Size * 1.6f, Size * 1.35f, Frame, Color);
        }

        if (!Detailed && Offscreen)
        {
            const glm::vec2 MarkerDirection = RawPosition - PanelCenter;
            if (glm::length(MarkerDirection) > 0.001f)
            {
                const glm::vec2 Unit = glm::normalize(MarkerDirection);
                std::vector<glm::vec2> Tick;
                AppendLine(Tick, Position - Unit * 5.0f, Position, 1.5f);
                DrawUiTriangles(Tick, Frame);
            }
        }

        if (Detailed)
        {
            const float DeltaX = Position.x - MenuPointerX;
            const float DeltaY = Position.y - MenuPointerY;
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
        const glm::vec2 RawPosition = ToScreen(Waypoint.Position.x, Waypoint.Position.y);
        bool Offscreen = false;
        const glm::vec2 Position = ClampCompactMarker(RawPosition, 12.0f, Offscreen);
        const float BaseRadius = Detailed
            ? static_cast<float>(GeneratedWebUi::FullMapWaypointSize) * 0.5f
            : static_cast<float>(GeneratedWebUi::MiniMapWaypointFont) * 0.45f;
        const float Pulse = Detailed
            ? 1.0f + (std::sin(Time * 4.6f) + 1.0f) * 1.4f
            : 0.0f;

        DrawCircle(Position, BaseRadius + 3.0f + Pulse, Frame);
        DrawCircle(Position, BaseRadius + 1.0f + Pulse, WaypointColor);
        DrawCircle(Position, std::max(BaseRadius - 2.0f, 2.0f), Background);

        if (!Detailed && Offscreen)
        {
            glm::vec2 WaypointDirection = RawPosition - PanelCenter;
            if (glm::length(WaypointDirection) < 0.001f)
                WaypointDirection = {0.0f, -1.0f};
            DrawArrow(Position, WaypointDirection, 9.0f, 7.0f, Frame, WaypointColor);
        }

        if (Detailed)
        {
            const float DeltaX = Position.x - MenuPointerX;
            const float DeltaY = Position.y - MenuPointerY;
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

    const glm::vec2 PlayerScreen = ToScreen(PlayerPosition.x, PlayerPosition.y);
    glm::vec2 Direction = Detailed ? PlayerForward : glm::vec2{0.0f, -1.0f};
    const float PlayerLength = Detailed
        ? static_cast<float>(GeneratedWebUi::FullMapPlayerSize)
        : static_cast<float>(GeneratedWebUi::MiniMapPlayerFont);

    DrawArrow(
        PlayerScreen,
        Direction,
        PlayerLength,
        PlayerLength * 0.72f,
        Frame,
        PlayerColor
    );
    DrawCircle(PlayerScreen, Detailed ? 2.6f : 2.0f, RouteColor);

    glDisable(GL_SCISSOR_TEST);

    if (Detailed && !HoverLabel.empty() && MenuTextRenderer.IsReady())
    {
        const int FontSize = GeneratedWebUi::FullMapMetaFont;
        const int TextWidth = MenuTextRenderer.Measure(
            HoverLabel,
            FontSize,
            GeneratedWebUi::FullMapMetaWeight,
            GeneratedWebUi::FullMapMetaTracking
        );
        int LabelX = static_cast<int>(std::round(HoverScreen.x)) + 16;
        int LabelY = static_cast<int>(std::round(HoverScreen.y)) - 28;

        if (LabelX + TextWidth + 20 > X + PanelWidth)
            LabelX = static_cast<int>(std::round(HoverScreen.x)) - TextWidth - 28;
        if (LabelY < Y + 8)
            LabelY = static_cast<int>(std::round(HoverScreen.y)) + 18;

        DrawRect(LabelX - 8, LabelY - 6, TextWidth + 16, FontSize + 14, Frame);
        MenuTextRenderer.Draw(
            HoverLabel,
            LabelX,
            LabelY,
            FontSize,
            GeneratedWebUi::FullMapMetaWeight,
            GeneratedWebUi::FullMapMetaTracking,
            PlayerColor,
            1.0f,
            false
        );
    }

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
    auto CssColor = [](const GeneratedWebUi::CssColor& Color)
    {
        return glm::vec3{Color.R, Color.G, Color.B};
    };

    const bool Compact = Width < 900 || Height < 620;
    const int MapWidth = std::clamp(
        static_cast<int>(Width * (Compact ? 0.30f : 0.20f)),
        Compact ? 224 : 286,
        Compact ? 286 : 342
    );
    const int MapHeight = std::clamp(
        static_cast<int>(Height * (Compact ? 0.235f : 0.215f)),
        Compact ? 154 : 180,
        Compact ? 198 : 214
    );
    const int Margin = std::clamp(static_cast<int>(Width * 0.015f), 16, 26);
    const int MapX = Margin;
    const int MapY = static_cast<int>(Height) - MapHeight - Margin;
    const int HeaderHeight = 31;
    const int FooterHeight = Waypoint.Active ? 27 : 0;
    const int FrameThickness = 2;
    const int InnerPadding = 7;

    const int InnerX = MapX + FrameThickness + InnerPadding;
    const int InnerY = MapY + HeaderHeight + InnerPadding;
    const int InnerWidth = std::max(MapWidth - (FrameThickness + InnerPadding) * 2, 1);
    const int InnerHeight = std::max(
        MapHeight - HeaderHeight - FooterHeight - FrameThickness - InnerPadding * 2,
        1
    );

    const glm::vec3 Frame = CssColor(GeneratedWebUi::MapFrameColor);
    const glm::vec3 PanelFill = CssColor(GeneratedWebUi::MapBackgroundColor);
    const glm::vec3 WarmText = CssColor(GeneratedWebUi::MiniMapPlayerColor);
    const glm::vec3 RouteAccent = CssColor(GeneratedWebUi::MiniMapRouteColor);
    const glm::vec3 HeaderIdle{0.105f, 0.096f, 0.046f};
    const glm::vec3 HeaderAlert{0.40f, 0.025f, 0.018f};
    const bool ThreatNear = std::isfinite(ThreatDistance) && ThreatDistance < 36.0f;
    const bool RedPhase = ThreatNear && std::fmod(Time * 7.0f, 1.0f) < 0.5f;
    const glm::vec3 HeaderFill = RedPhase ? HeaderAlert : HeaderIdle;

    DrawRect(MapX + 4, MapY + 4, MapWidth, MapHeight, {0.055f, 0.050f, 0.026f});
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
        Compact ? 2.55f : 2.80f,
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
            : glm::vec3{0.08f, 0.03f, 0.018f};
        const int Border = RedPhase ? 3 : 2;
        DrawRect(MapX - Border, MapY - Border, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY + MapHeight, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY, Border, MapHeight, Alert);
        DrawRect(MapX + MapWidth, MapY, Border, MapHeight, Alert);
    }

    if (MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Draw(
            ThreatNear ? "THREAT" : "MAP",
            MapX + 11,
            MapY + 7,
            11,
            700,
            0.06f,
            ThreatNear ? glm::vec3{1.0f, 0.76f, 0.62f} : WarmText,
            1.0f,
            false
        );

        MenuTextRenderer.Draw(
            "LEVEL 0",
            MapX + 62,
            MapY + 8,
            9,
            600,
            0.04f,
            WarmText,
            0.78f,
            false
        );

        const std::string ExpandHint = "M  EXPAND";
        const int ExpandWidth = MenuTextRenderer.Measure(ExpandHint, 9, 600, 0.04f);
        MenuTextRenderer.Draw(
            ExpandHint,
            MapX + MapWidth - 11 - ExpandWidth,
            MapY + 8,
            9,
            600,
            0.04f,
            WarmText,
            0.82f,
            false
        );

        if (Waypoint.Active)
        {
            std::ostringstream Distance;
            Distance
                << "WAYPOINT  "
                << std::fixed
                << std::setprecision(0)
                << Waypoint.DistanceMeters
                << " M";

            MenuTextRenderer.Draw(
                Distance.str(),
                MapX + 11,
                MapY + MapHeight - FooterHeight + 7,
                9,
                650,
                0.035f,
                WarmText,
                1.0f,
                false
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
    auto CssColor = [](const GeneratedWebUi::CssColor& Color)
    {
        return glm::vec3{Color.R, Color.G, Color.B};
    };

    glDisable(GL_DEPTH_TEST);

    const glm::vec3 Background = CssColor(GeneratedWebUi::MapBackgroundColor);
    const glm::vec3 Ink = CssColor(GeneratedWebUi::FullMapTitleColor);
    const glm::vec3 Muted = CssColor(GeneratedWebUi::FullMapMetaColor);
    const glm::vec3 Frame = CssColor(GeneratedWebUi::MapFrameColor);
    const glm::vec3 ButtonIdle{0.76f, 0.71f, 0.35f};
    const glm::vec3 ButtonHover{0.16f, 0.145f, 0.070f};
    const glm::vec3 ButtonHoverText{0.96f, 0.93f, 0.73f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Background);
    DrawRect(0, 0, static_cast<int>(Width), 1, Frame);

    const bool Compact = Width < 930 || Height < 650;
    const int Margin = Compact ? 16 : 28;
    const int ButtonHeight = Compact ? 36 : 40;
    const int HeaderHeight = Compact ? 98 : 112;
    const int ButtonY = 16;
    const int Gap = Compact ? 8 : 10;

    const int BackWidth = Compact ? 112 : 142;
    const int MainMenuWidth = Compact ? 122 : 156;
    const int RandomWidth = Compact ? 152 : 194;
    const int ClearWidth = Compact ? 118 : 150;

    FullMapBackRect = {Margin, ButtonY, BackWidth, ButtonHeight};
    FullMapMainMenuRect = {Margin + BackWidth + Gap, ButtonY, MainMenuWidth, ButtonHeight};
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
            Hovered ? CssColor(GeneratedWebUi::FullMapRouteColor) : Frame
        );

        if (MenuTextRenderer.IsReady())
        {
            MenuTextRenderer.Draw(
                Label,
                Rect.X + 14,
                Rect.Y + (Rect.Height - 11) / 2,
                11,
                650,
                0.02f,
                TextColor,
                1.0f,
                false
            );
        }
    };

    DrawButton(FullMapBackRect, "RESUME", FullMapBackHover);
    DrawButton(FullMapMainMenuRect, "MAIN MENU", FullMapMainMenuHover);
    DrawButton(FullMapRandomRect, "RANDOM DESTINATION", FullMapRandomHover);
    DrawButton(FullMapClearRect, "CLEAR WAYPOINT", FullMapClearHover);

    const int TitleY = ButtonY + ButtonHeight + (Compact ? 10 : 12);

    if (MenuTextRenderer.IsReady())
    {
        MenuTextRenderer.Draw(
            "LEVEL 0",
            Margin,
            TitleY,
            GeneratedWebUi::FullMapTitleFont,
            GeneratedWebUi::FullMapTitleWeight,
            GeneratedWebUi::FullMapTitleTracking,
            Ink,
            1.0f,
            false
        );

        const int MapLabelX = Margin + MenuTextRenderer.Measure(
            "LEVEL 0",
            GeneratedWebUi::FullMapTitleFont,
            GeneratedWebUi::FullMapTitleWeight,
            GeneratedWebUi::FullMapTitleTracking
        ) + 18;

        MenuTextRenderer.Draw(
            "MAP / LIVE FLOORPLAN",
            MapLabelX,
            TitleY + 9,
            GeneratedWebUi::FullMapMetaFont,
            GeneratedWebUi::FullMapMetaWeight,
            GeneratedWebUi::FullMapMetaTracking,
            Muted,
            1.0f,
            false
        );

        std::ostringstream Status;
        Status << "POWER " << BreakersActive << "/" << BreakersRequired;
        if (Waypoint.Active)
            Status << "   WAYPOINT " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";

        const int StatusWidth = MenuTextRenderer.Measure(
            Status.str(),
            GeneratedWebUi::FullMapMetaFont,
            GeneratedWebUi::FullMapMetaWeight,
            GeneratedWebUi::FullMapMetaTracking
        );
        MenuTextRenderer.Draw(
            Status.str(),
            std::max(Margin, static_cast<int>(Width) - Margin - StatusWidth),
            TitleY + 9,
            GeneratedWebUi::FullMapMetaFont,
            GeneratedWebUi::FullMapMetaWeight,
            GeneratedWebUi::FullMapMetaTracking,
            Muted,
            1.0f,
            false
        );
    }

    const int FooterHeight = Compact ? 50 : 58;
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

    if (MenuTextRenderer.IsReady())
    {
        std::ostringstream ZoomText;
        ZoomText
            << "ZOOM "
            << std::fixed
            << std::setprecision(0)
            << std::clamp(Zoom, 0.45f, 4.0f) * 100.0f
            << "%";

        const int FooterY = static_cast<int>(Height) - FooterHeight + 11;
        const int Font = Compact ? 8 : 9;

        MenuTextRenderer.Draw(
            "CLICK SETS WAYPOINT  /  DRAG PANS  /  WHEEL ZOOMS  /  R RECENTERS  /  G RANDOM  /  ESC RESUMES",
            Margin,
            FooterY,
            Font,
            600,
            0.025f,
            Muted,
            1.0f,
            false
        );

        MenuTextRenderer.Draw(
            "OFF-SCREEN OBJECTIVES STAY PINNED TO THE MINIMAP EDGE",
            Margin,
            FooterY + 17,
            Font,
            550,
            0.02f,
            Muted,
            0.86f,
            false
        );

        const int ZoomWidth = MenuTextRenderer.Measure(ZoomText.str(), Font, 650, 0.03f);
        MenuTextRenderer.Draw(
            ZoomText.str(),
            static_cast<int>(Width) - Margin - ZoomWidth,
            FooterY + 17,
            Font,
            650,
            0.03f,
            Ink,
            1.0f,
            false
        );
    }

    glEnable(GL_DEPTH_TEST);
}


MapUiAction Renderer::HitTestFullMap() const
{
    if (FullMapBackRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::Back;
    if (FullMapMainMenuRect.Contains(MenuPointerX, MenuPointerY))
        return MapUiAction::MainMenu;
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
