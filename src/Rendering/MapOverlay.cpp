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
    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 WallpaperA{0.748f, 0.696f, 0.346f};
    const glm::vec3 WallpaperB{0.790f, 0.735f, 0.366f};
    const glm::vec3 Wall{0.125f, 0.113f, 0.050f};
    const glm::vec3 RouteColor{0.98f, 0.95f, 0.72f};

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

    for (int StripeX = X + 17; StripeX < X + PanelWidth; StripeX += 34)
        ClipRect(StripeX, Y, 2, PanelHeight, WallpaperA);

    for (int StripeX = X + 26; StripeX < X + PanelWidth; StripeX += 68)
        ClipRect(StripeX, Y, 1, PanelHeight, WallpaperB);

    for (int StripeY = Y + 23; StripeY < Y + PanelHeight; StripeY += 46)
    {
        for (int MotifX = X + 8; MotifX < X + PanelWidth; MotifX += 34)
        {
            ClipRect(MotifX, StripeY, 7, 1, WallpaperB);
            ClipRect(MotifX + 3, StripeY - 2, 1, 5, WallpaperA);
        }
    }

    const float Scale = std::max(PixelsPerMeter, 0.1f);
    const float HalfCell = World.CellSize * 0.5f;

    auto ToScreen = [&](float WorldX, float WorldZ)
    {
        return glm::ivec2{
            X + PanelWidth / 2 +
                static_cast<int>(std::round((WorldX - MapCenter.x) * Scale)),
            Y + PanelHeight / 2 +
                static_cast<int>(std::round((WorldZ - MapCenter.y) * Scale))
        };
    };

    const int WallThickness = Detailed ? 2 : 1;

    for (int LocalZ = 0; LocalZ < World.Rows; ++LocalZ)
    {
        for (int LocalX = 0; LocalX < World.Columns; ++LocalX)
        {
            const MazeCell& Cell = World.Cell(LocalX, LocalZ);
            const float CenterX = static_cast<float>(Cell.X) * World.CellSize;
            const float CenterZ = static_cast<float>(Cell.Z) * World.CellSize;
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

            if (Detailed && CellWidth > 5 && CellHeight > 5)
            {
                const int TintIndex = std::abs(Cell.X * 29 + Cell.Z * 43) % 4;
                const glm::vec3 Tint =
                    TintIndex == 0
                        ? glm::vec3{0.785f, 0.730f, 0.360f}
                        : TintIndex == 1
                            ? glm::vec3{0.800f, 0.744f, 0.378f}
                            : TintIndex == 2
                                ? glm::vec3{0.765f, 0.710f, 0.340f}
                                : glm::vec3{0.810f, 0.755f, 0.390f};

                ClipRect(
                    TopLeft.x + 1,
                    TopLeft.y + 1,
                    std::max(CellWidth - 2, 1),
                    std::max(CellHeight - 2, 1),
                    Tint
                );
            }

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
            const glm::vec2 Segment = glm::vec2(B - A);
            const float Length = glm::length(Segment);
            const int DotCount = std::max(static_cast<int>(Length / (Detailed ? 9.0f : 7.0f)), 1);

            for (int Dot = 0; Dot <= DotCount; ++Dot)
            {
                const float T = static_cast<float>(Dot) / static_cast<float>(DotCount);
                const int DotX = static_cast<int>(std::round(A.x + (B.x - A.x) * T));
                const int DotY = static_cast<int>(std::round(A.y + (B.y - A.y) * T));
                const int Size = Detailed ? 4 : 3;
                ClipRect(DotX - Size / 2, DotY - Size / 2, Size, Size, RouteColor);
            }
        }
    }

    float HoverDistance = std::numeric_limits<float>::max();
    std::string HoverLabel;
    glm::ivec2 HoverScreen{0};

    for (const MapMarker& Marker : Markers)
    {
        const glm::ivec2 Position = ToScreen(Marker.Position.x, Marker.Position.y);

        if (
            Position.x < X - 18 ||
            Position.y < Y - 18 ||
            Position.x > X + PanelWidth + 18 ||
            Position.y > Y + PanelHeight + 18
        )
        {
            continue;
        }

        if (Marker.Kind == MapMarkerKind::Breaker || Marker.Kind == MapMarkerKind::BreakerActive)
        {
            const bool Active = Marker.Kind == MapMarkerKind::BreakerActive;
            const glm::vec3 Glow = Active
                ? glm::vec3{0.14f, 0.95f, 0.30f}
                : glm::vec3{1.0f, 0.10f, 0.055f};
            const int Radius = Detailed ? 9 : 7;
            Ring(Position.x, Position.y, Radius + 2, 2, Ink);
            Ring(Position.x, Position.y, Radius, 3, Glow);
            Line(
                {Position.x - 3, Position.y + 4},
                {Position.x + 4, Position.y - 4},
                2,
                Glow
            );
            ClipRect(Position.x + 3, Position.y - 6, 5, 3, Glow);
            ClipRect(Position.x - 5, Position.y + 3, 4, 4, Glow);
        }
        else if (Marker.Kind == MapMarkerKind::Exit || Marker.Kind == MapMarkerKind::ExitPowered)
        {
            const glm::vec3 Color =
                Marker.Kind == MapMarkerKind::ExitPowered
                    ? glm::vec3{0.18f, 0.98f, 0.46f}
                    : glm::vec3{0.96f, 0.93f, 0.73f};
            const int Size = Detailed ? 13 : 10;
            ClipRect(Position.x - Size / 2 - 2, Position.y - Size / 2 - 2, Size + 4, Size + 4, Ink);
            ClipRect(Position.x - Size / 2, Position.y - Size / 2, Size, Size, Color);
            ClipRect(Position.x + Size / 4, Position.y, 2, 2, Ink);
        }
        else
        {
            const bool RedPhase = std::fmod(Time * 6.0f, 1.0f) < 0.55f;
            const glm::vec3 Threat = RedPhase
                ? glm::vec3{1.0f, 0.03f, 0.02f}
                : Ink;
            const int Radius = Detailed ? 8 : 6;
            FillCircle(Position.x, Position.y, Radius + 3, Ink);
            FillCircle(Position.x, Position.y, Radius, Threat);
            ClipRect(Position.x - 1, Position.y - Radius - 5, 3, 4, Threat);
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
        const int Pulse = 9 + static_cast<int>((std::sin(Time * 5.0f) + 1.0f) * 2.0f);
        Ring(Position.x, Position.y, Pulse + 3, 2, Ink);
        Ring(Position.x, Position.y, Pulse, 2, RouteColor);
        Line({Position.x - 5, Position.y}, {Position.x + 5, Position.y}, 2, RouteColor);
        Line({Position.x, Position.y - 5}, {Position.x, Position.y + 5}, 2, RouteColor);

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
    glm::vec2 Direction = PlayerForward;

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const glm::vec2 Right{Direction.y, -Direction.x};
    const int ArrowLength = Detailed ? 13 : 10;
    const int ArrowWidth = Detailed ? 7 : 5;
    const glm::ivec2 Tip{
        PlayerScreen.x + static_cast<int>(std::round(Direction.x * ArrowLength)),
        PlayerScreen.y + static_cast<int>(std::round(Direction.y * ArrowLength))
    };
    const glm::ivec2 Left{
        PlayerScreen.x - static_cast<int>(std::round(Direction.x * 5.0f)) + static_cast<int>(std::round(Right.x * ArrowWidth)),
        PlayerScreen.y - static_cast<int>(std::round(Direction.y * 5.0f)) + static_cast<int>(std::round(Right.y * ArrowWidth))
    };
    const glm::ivec2 RightPoint{
        PlayerScreen.x - static_cast<int>(std::round(Direction.x * 5.0f)) - static_cast<int>(std::round(Right.x * ArrowWidth)),
        PlayerScreen.y - static_cast<int>(std::round(Direction.y * 5.0f)) - static_cast<int>(std::round(Right.y * ArrowWidth))
    };

    Line(Tip, Left, 5, Ink);
    Line(Tip, RightPoint, 5, Ink);
    Line(Left, RightPoint, 5, Ink);
    Line(Tip, Left, 2, {1.0f, 0.98f, 0.82f});
    Line(Tip, RightPoint, 2, {1.0f, 0.98f, 0.82f});
    Line(Left, RightPoint, 2, {1.0f, 0.98f, 0.82f});

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

    DrawRect(X, Y, PanelWidth, 2, Ink);
    DrawRect(X, Y + PanelHeight - 2, PanelWidth, 2, Ink);
    DrawRect(X, Y, 2, PanelHeight, Ink);
    DrawRect(X + PanelWidth - 2, Y, 2, PanelHeight, Ink);
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
    const int MapWidth = std::clamp(static_cast<int>(Width * (Compact ? 0.26f : 0.18f)), 190, 300);
    const int MapHeight = std::clamp(static_cast<int>(Height * (Compact ? 0.23f : 0.20f)), 124, 190);
    const int Margin = std::clamp(static_cast<int>(Width * 0.016f), 14, 28);
    const int MapX = Margin;
    const int MapY = static_cast<int>(Height) - MapHeight - Margin;

    DrawMapPanelV2(
        World,
        MapX,
        MapY,
        MapWidth,
        MapHeight,
        PlayerPosition,
        Compact ? 1.38f : 1.58f,
        PlayerPosition,
        PlayerForward,
        Markers,
        Waypoint,
        Route,
        false,
        Time
    );

    const glm::vec3 Ink = LevelZeroInk();
    const bool ThreatNear = std::isfinite(ThreatDistance) && ThreatDistance < 36.0f;

    if (ThreatNear)
    {
        const bool RedPhase = std::fmod(Time * 7.0f, 1.0f) < 0.5f;
        const glm::vec3 Alert = RedPhase
            ? glm::vec3{0.98f, 0.025f, 0.015f}
            : glm::vec3{0.015f, 0.012f, 0.008f};
        const int Border = RedPhase ? 5 : 4;
        DrawRect(MapX - Border, MapY - Border, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY + MapHeight, MapWidth + Border * 2, Border, Alert);
        DrawRect(MapX - Border, MapY, Border, MapHeight, Alert);
        DrawRect(MapX + MapWidth, MapY, Border, MapHeight, Alert);
    }

    if (GameplayTextRenderer.IsReady())
    {
        GameplayTextRenderer.Resize(Width, Height);
        GameplayTextRenderer.Draw(
            ThreatNear ? "THREAT NEAR" : "M  MAP",
            MapX + 10,
            MapY + 9,
            9,
            850,
            0.09f,
            ThreatNear ? glm::vec3{0.78f, 0.025f, 0.015f} : Ink,
            1.0f,
            false
        );

        if (Waypoint.Active)
        {
            std::ostringstream Distance;
            Distance << "WAYPOINT  " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";
            GameplayTextRenderer.Draw(
                Distance.str(),
                MapX + 10,
                MapY + MapHeight - 21,
                9,
                800,
                0.05f,
                Ink,
                1.0f,
                false
            );
        }
    }
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

    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 Muted{0.36f, 0.32f, 0.14f};
    const glm::vec3 ButtonHover{0.69f, 0.63f, 0.28f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Yellow);

    for (int X = 15; X < static_cast<int>(Width); X += 38)
        DrawRect(X, 0, 1, static_cast<int>(Height), {0.745f, 0.690f, 0.337f});

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
        const glm::vec3 Fill = Hover > 0.01f ? ButtonHover : Yellow;
        DrawRect(Rect.X, Rect.Y, Rect.Width, Rect.Height, Fill);
        DrawRect(Rect.X, Rect.Y, Rect.Width, 2, Ink);
        DrawRect(Rect.X, Rect.Y + Rect.Height - 2, Rect.Width, 2, Ink);
        DrawRect(Rect.X, Rect.Y, 2, Rect.Height, Ink);
        DrawRect(Rect.X + Rect.Width - 2, Rect.Y, 2, Rect.Height, Ink);

        GameplayTextRenderer.Draw(
            Label,
            Rect.X + 12,
            Rect.Y + (Rect.Height - 11) / 2,
            11,
            850,
            0.06f,
            Ink,
            1.0f,
            false
        );
    };

    DrawButton(FullMapBackRect, "<  BACK", FullMapBackHover);
    DrawButton(FullMapRandomRect, "RANDOM DESTINATION", FullMapRandomHover);
    DrawButton(FullMapClearRect, "CLEAR MARKER", FullMapClearHover);

    const int TitleY = ButtonY + ButtonHeight + (Compact ? 10 : 12);
    GameplayTextRenderer.Draw(
        "LEVEL 0 MAP",
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
        "ESC BACK TO MENU   |   CLICK SET MARKER   |   DRAG PAN   |   WHEEL ZOOM   |   R RECENTER   |   G RANDOM",
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
