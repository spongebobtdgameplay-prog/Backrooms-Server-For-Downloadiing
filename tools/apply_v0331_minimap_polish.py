from pathlib import Path
import re


def replace_exact(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"missing exact block: {label}")
    return text.replace(old, new, 1)


def replace_regex(text: str, pattern: str, replacement: str, label: str) -> str:
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise RuntimeError(f"regex replacement failed for {label}: {count}")
    return updated


map_path = Path("src/Rendering/MapOverlay.cpp")
map_text = map_path.read_text(encoding="utf-8")

old_background = '''    DrawRect(X, Y, PanelWidth, PanelHeight, Yellow);

    for (int StripeX = X + 28; StripeX < X + PanelWidth; StripeX += 56)
        ClipRect(StripeX, Y, 1, PanelHeight, WallpaperLine);

    for (int StripeX = X + 42; StripeX < X + PanelWidth; StripeX += 112)
        ClipRect(StripeX, Y, 1, PanelHeight, WallpaperAccent);

    for (int MotifY = Y + 34; MotifY < Y + PanelHeight; MotifY += 68)
    {
        for (int MotifX = X + 14; MotifX < X + PanelWidth; MotifX += 56)
            ClipRect(MotifX, MotifY, 8, 1, WallpaperLine);
    }
'''
new_background = '''    DrawRect(X, Y, PanelWidth, PanelHeight, Yellow);

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
'''
map_text = replace_exact(map_text, old_background, new_background, "map background")
map_text = replace_exact(map_text, "    const int WallThickness = 1;", "    const int WallThickness = Detailed ? 1 : 2;", "minimap wall thickness")
map_text = replace_exact(
    map_text,
    '''            Line(A, B, Detailed ? 6 : 4, RouteOuter);
            Line(A, B, Detailed ? 3 : 2, RouteColor);''',
    '''            Line(A, B, Detailed ? 6 : 5, RouteOuter);
            Line(A, B, 3, RouteColor);''',
    "route thickness"
)
map_text = replace_exact(map_text, "        const int MarkerSafeInset = Detailed ? 0 : 12;", "        const int MarkerSafeInset = Detailed ? 0 : 16;", "marker safe inset")
map_text = map_text.replace("const int Radius = Detailed ? 8 : 6;", "const int Radius = Detailed ? 8 : 8;", 2)
map_text = replace_exact(map_text, "            const int Radius = Detailed ? 9 : 7;", "            const int Radius = Detailed ? 9 : 9;", "entity marker size")
map_text = replace_exact(map_text, "    const int ArrowLength = Detailed ? 14 : 11;", "    const int ArrowLength = Detailed ? 14 : 16;", "player arrow length")
map_text = replace_exact(map_text, "    const int ArrowWidth = Detailed ? 8 : 6;", "    const int ArrowWidth = Detailed ? 8 : 9;", "player arrow width")
map_text = replace_exact(map_text, "    FillCircle(PlayerScreen.x, PlayerScreen.y, Detailed ? 3 : 2, RouteColor);", "    FillCircle(PlayerScreen.x, PlayerScreen.y, 3, RouteColor);", "player center dot")

old_frame = '''    const glm::vec3 Frame{0.34f, 0.31f, 0.15f};
    DrawRect(X, Y, PanelWidth, 1, Frame);
    DrawRect(X, Y + PanelHeight - 1, PanelWidth, 1, Frame);
    DrawRect(X, Y, 1, PanelHeight, Frame);
    DrawRect(X + PanelWidth - 1, Y, 1, PanelHeight, Frame);
}'''
new_frame = '''    const glm::vec3 Frame{0.24f, 0.22f, 0.10f};
    const int FrameThickness = Detailed ? 1 : 2;
    DrawRect(X, Y, PanelWidth, FrameThickness, Frame);
    DrawRect(X, Y + PanelHeight - FrameThickness, PanelWidth, FrameThickness, Frame);
    DrawRect(X, Y, FrameThickness, PanelHeight, Frame);
    DrawRect(X + PanelWidth - FrameThickness, Y, FrameThickness, PanelHeight, Frame);
}'''
map_text = replace_exact(map_text, old_frame, new_frame, "map panel frame")

new_minimap = r'''void Renderer::DrawMiniMapV2(
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
'''

map_text = replace_regex(
    map_text,
    r'void Renderer::DrawMiniMapV2\([\s\S]*?\n\}\n\nvoid Renderer::DrawFullMapV2',
    new_minimap + '\nvoid Renderer::DrawFullMapV2',
    "DrawMiniMapV2"
)
map_path.write_text(map_text, encoding="utf-8")


interactive_path = Path("src/Rendering/InteractiveMenu.cpp")
interactive = interactive_path.read_text(encoding="utf-8")
new_sprint = r'''    const bool CompactHud = Width < 900 || Height < 620;
    const int MiniMapHeight = std::clamp(
        static_cast<int>(Height * (CompactHud ? 0.22f : 0.205f)),
        CompactHud ? 148 : 172,
        CompactHud ? 188 : 202
    );
    const int MiniMapMargin = std::clamp(
        static_cast<int>(Width * 0.015f),
        16,
        26
    );
    const int MiniMapTop =
        static_cast<int>(Height) - MiniMapHeight - MiniMapMargin;

    const int SprintFontHeight = 11;
    const int SprintY = std::max(80, MiniMapTop - 36);

    GameplayTextRenderer.Draw(
        "SPRINT",
        LeftX,
        SprintY,
        SprintFontHeight,
        780,
        0.15f,
        StaminaColor,
        1.0f,
        true
    );

    const int SprintLabelWidth =
        GameplayTextRenderer.Measure(
            "SPRINT",
            SprintFontHeight,
            780,
            0.15f
        );

    const int BarX = LeftX + SprintLabelWidth + 12;
    const int BarWidth = 148;
    const int BarHeight = 6;
    const int TextRasterTopPadding = 4;
    const int BarY =
        SprintY +
        TextRasterTopPadding +
        (SprintFontHeight - BarHeight) / 2;

    const int FillWidth =
        static_cast<int>(
            std::round(
                static_cast<float>(BarWidth) *
                StaminaAmount
            )
        );

    DrawRect(
        BarX - 1,
        BarY - 1,
        BarWidth + 2,
        BarHeight + 2,
        {0.07f, 0.065f, 0.035f}
    );

    DrawRect(
        BarX,
        BarY,
        BarWidth,
        BarHeight,
        {0.18f, 0.16f, 0.085f}
    );

    if (FillWidth > 0)
    {
        DrawRect(
            BarX,
            BarY,
            FillWidth,
            BarHeight,
            StaminaColor
        );
    }

    std::string Prompt;'''
interactive = replace_regex(
    interactive,
    r'    const int SprintFontHeight = 10;[\s\S]*?\n    std::string Prompt;',
    new_sprint,
    "sprint/minimap composition"
)
interactive_path.write_text(interactive, encoding="utf-8")


renderer_path = Path("src/Rendering/Renderer.cpp")
renderer = renderer_path.read_text(encoding="utf-8")
new_crosshair = r'''void Renderer::DrawCrosshair()
{
    const int CenterX = static_cast<int>(Width / 2);
    const int CenterY = static_cast<int>(Height / 2);

    const glm::vec3 Stroke{
        0.035f,
        0.032f,
        0.020f
    };

    const glm::vec3 Reticle{
        1.0f,
        0.965f,
        0.72f
    };

    const int Gap = 8;
    const int TickLength = 9;
    const int StrokeThickness = 4;
    const int TickThickness = 2;

    DrawRect(
        CenterX - Gap - TickLength - 1,
        CenterY - StrokeThickness / 2,
        TickLength + 2,
        StrokeThickness,
        Stroke
    );
    DrawRect(
        CenterX + Gap - 1,
        CenterY - StrokeThickness / 2,
        TickLength + 2,
        StrokeThickness,
        Stroke
    );
    DrawRect(
        CenterX - StrokeThickness / 2,
        CenterY - Gap - TickLength - 1,
        StrokeThickness,
        TickLength + 2,
        Stroke
    );
    DrawRect(
        CenterX - StrokeThickness / 2,
        CenterY + Gap - 1,
        StrokeThickness,
        TickLength + 2,
        Stroke
    );

    DrawRect(
        CenterX - Gap - TickLength,
        CenterY - TickThickness / 2,
        TickLength,
        TickThickness,
        Reticle
    );
    DrawRect(
        CenterX + Gap,
        CenterY - TickThickness / 2,
        TickLength,
        TickThickness,
        Reticle
    );
    DrawRect(
        CenterX - TickThickness / 2,
        CenterY - Gap - TickLength,
        TickThickness,
        TickLength,
        Reticle
    );
    DrawRect(
        CenterX - TickThickness / 2,
        CenterY + Gap,
        TickThickness,
        TickLength,
        Reticle
    );

    DrawRect(CenterX - 2, CenterY - 2, 5, 5, Stroke);
    DrawRect(CenterX - 1, CenterY - 1, 3, 3, Reticle);
}
'''
renderer = replace_regex(
    renderer,
    r'void Renderer::DrawCrosshair\(\)\n\{[\s\S]*?\n\}\n\nvoid Renderer::DrawStamina',
    new_crosshair + '\nvoid Renderer::DrawStamina',
    "crosshair"
)
renderer_path.write_text(renderer, encoding="utf-8")


cmake_path = Path("CMakeLists.txt")
cmake = cmake_path.read_text(encoding="utf-8")
cmake = replace_exact(
    cmake,
    "project(BackroomsOffical VERSION 0.3.30 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.31 LANGUAGES C CXX)",
    "CMake version"
)
cmake_path.write_text(cmake, encoding="utf-8")

version_path = Path("src/Core/Version.h")
version = version_path.read_text(encoding="utf-8")
version = replace_exact(version, 'Text = "0.3.30"', 'Text = "0.3.31"', "Version.h")
version_path.write_text(version, encoding="utf-8")

rc_path = Path("src/Platform/Windows/Backrooms.rc")
rc = rc_path.read_text(encoding="utf-8")
if "0,3,30,0" not in rc or "0.3.30" not in rc:
    raise RuntimeError("expected V0.3.30 Windows metadata not found")
rc = rc.replace("0,3,30,0", "0,3,31,0")
rc = rc.replace("0.3.30", "0.3.31")
rc_path.write_text(rc, encoding="utf-8")

notes = (
    "V0.3.31 rebuilds the gameplay minimap presentation. The minimap is larger and uses a clean dark HUD frame with a dedicated MAP / LEVEL 0 header, a separate waypoint footer, an inset maze viewport, stronger local zoom, thicker readable maze walls, larger breaker/entity/player markers, and a clearer blue route. Marker clipping is prevented by safe inner bounds. The sprint meter is positioned from the minimap top so the two HUD elements always keep deliberate spacing. The center reticle is also larger with separated high-contrast ticks and a stronger center dot."
)
Path("update/release_notes.txt").write_text(notes, encoding="utf-8")

print("V0.3.31 minimap polish applied")
