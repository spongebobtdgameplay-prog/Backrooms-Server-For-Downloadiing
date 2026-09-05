from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    (ROOT / path).write_text(text, encoding="utf-8", newline="\n")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)


def sub_once(text, pattern, replacement, label):
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one regex match, got {count}")
    return updated


map_path = "src/Rendering/MapOverlay.cpp"
text = read(map_path)

text = replace_once(
    text,
    """    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 WallpaperA{0.748f, 0.696f, 0.346f};
    const glm::vec3 WallpaperB{0.790f, 0.735f, 0.366f};
    const glm::vec3 Wall{0.125f, 0.113f, 0.050f};
    const glm::vec3 RouteColor{0.98f, 0.95f, 0.72f};
""",
    """    const glm::vec3 Yellow{0.815f, 0.765f, 0.420f};
    const glm::vec3 Ink{0.105f, 0.095f, 0.045f};
    const glm::vec3 WallpaperLine{0.765f, 0.712f, 0.360f};
    const glm::vec3 WallpaperAccent{0.845f, 0.795f, 0.455f};
    const glm::vec3 Wall{0.145f, 0.132f, 0.063f};
    const glm::vec3 RouteOuter{0.055f, 0.060f, 0.070f};
    const glm::vec3 RouteColor{0.245f, 0.545f, 0.965f};
""",
    "map palette",
)

line_needle = "    auto Line = [&](glm::ivec2 A, glm::ivec2 B, int Thickness, const glm::vec3& Color)\n"
triangle_helper = r'''    auto FillTriangle = [&](glm::ivec2 A, glm::ivec2 B, glm::ivec2 C, const glm::vec3& Color)
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

'''
text = replace_once(text, line_needle, triangle_helper + line_needle, "triangle helper")

text = sub_once(
    text,
    r"    DrawRect\(X, Y, PanelWidth, PanelHeight, Yellow\);\n\n.*?\n    const float Scale = std::max\(PixelsPerMeter, 0\.1f\);",
    """    DrawRect(X, Y, PanelWidth, PanelHeight, Yellow);

    for (int StripeX = X + 28; StripeX < X + PanelWidth; StripeX += 56)
        ClipRect(StripeX, Y, 1, PanelHeight, WallpaperLine);

    for (int StripeX = X + 42; StripeX < X + PanelWidth; StripeX += 112)
        ClipRect(StripeX, Y, 1, PanelHeight, WallpaperAccent);

    for (int MotifY = Y + 34; MotifY < Y + PanelHeight; MotifY += 68)
    {
        for (int MotifX = X + 14; MotifX < X + PanelWidth; MotifX += 56)
            ClipRect(MotifX, MotifY, 8, 1, WallpaperLine);
    }

    const float Scale = std::max(PixelsPerMeter, 0.1f);""",
    "map texture",
)

text = sub_once(
    text,
    r"\n            if \(Detailed && CellWidth > 5 && CellHeight > 5\)\n            \{.*?\n            \}\n\n            if \(Cell\.Walls\[0\]\)",
    "\n            if (Cell.Walls[0])",
    "remove cell patchwork",
)

text = replace_once(
    text,
    "    const int WallThickness = Detailed ? 2 : 1;",
    "    const int WallThickness = 1;",
    "wall thickness",
)

text = sub_once(
    text,
    r"    if \(Route\.size\(\) >= 2\)\n    \{.*?\n    \}\n\n    float HoverDistance",
    """    if (Route.size() >= 2)
    {
        for (std::size_t I = 1; I < Route.size(); ++I)
        {
            const glm::ivec2 A = ToScreen(Route[I - 1].x, Route[I - 1].y);
            const glm::ivec2 B = ToScreen(Route[I].x, Route[I].y);
            Line(A, B, Detailed ? 6 : 4, RouteOuter);
            Line(A, B, Detailed ? 3 : 2, RouteColor);
        }
    }

    float HoverDistance""",
    "continuous route",
)

text = sub_once(
    text,
    r"        if \(Marker\.Kind == MapMarkerKind::Breaker \|\| Marker\.Kind == MapMarkerKind::BreakerActive\)\n        \{.*?\n        \}\n        else if \(Marker\.Kind == MapMarkerKind::Exit \|\| Marker\.Kind == MapMarkerKind::ExitPowered\)\n        \{.*?\n        \}\n        else\n        \{.*?\n        \}\n\n        if \(Detailed\)",
    """        if (Marker.Kind == MapMarkerKind::Breaker || Marker.Kind == MapMarkerKind::BreakerActive)
        {
            const bool Active = Marker.Kind == MapMarkerKind::BreakerActive;
            const glm::vec3 Glow = Active
                ? glm::vec3{0.16f, 0.92f, 0.34f}
                : glm::vec3{1.0f, 0.13f, 0.08f};
            const int Radius = Detailed ? 8 : 6;

            FillCircle(Position.x, Position.y, Radius + 3, Ink);
            FillCircle(Position.x, Position.y, Radius + 1, Glow);
            FillCircle(Position.x, Position.y, std::max(Radius - 2, 2), Yellow);

            Line(
                {Position.x - 4, Position.y + 4},
                {Position.x + 3, Position.y - 3},
                2,
                Ink
            );
            FillCircle(Position.x + 4, Position.y - 4, 2, Ink);
            ClipRect(Position.x - 6, Position.y + 3, 5, 3, Ink);
        }
        else if (Marker.Kind == MapMarkerKind::Exit || Marker.Kind == MapMarkerKind::ExitPowered)
        {
            const glm::vec3 Color =
                Marker.Kind == MapMarkerKind::ExitPowered
                    ? glm::vec3{0.16f, 0.95f, 0.42f}
                    : glm::vec3{0.96f, 0.94f, 0.80f};
            const int Radius = Detailed ? 8 : 6;
            FillCircle(Position.x, Position.y, Radius + 2, Ink);
            FillCircle(Position.x, Position.y, Radius, Color);
            ClipRect(Position.x - 1, Position.y - 3, 2, 6, Ink);
            ClipRect(Position.x + 2, Position.y, 2, 2, Ink);
        }
        else
        {
            const bool RedPhase = std::fmod(Time * 5.0f, 1.0f) < 0.58f;
            const glm::vec3 Threat = RedPhase
                ? glm::vec3{1.0f, 0.035f, 0.02f}
                : glm::vec3{0.50f, 0.015f, 0.012f};
            const int Radius = Detailed ? 9 : 7;
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

        if (Detailed)""",
    "map markers",
)

text = sub_once(
    text,
    r"    const glm::ivec2 PlayerScreen = ToScreen\(PlayerPosition\.x, PlayerPosition\.y\);.*?\n    if \(Detailed && !HoverLabel\.empty\(\) && GameplayTextRenderer\.IsReady\(\)\)",
    """    const glm::ivec2 PlayerScreen = ToScreen(PlayerPosition.x, PlayerPosition.y);
    glm::vec2 Direction = PlayerForward;

    if (glm::length(Direction) < 0.001f)
        Direction = {0.0f, -1.0f};
    else
        Direction = glm::normalize(Direction);

    const glm::vec2 Right{Direction.y, -Direction.x};
    const int ArrowLength = Detailed ? 14 : 11;
    const int ArrowWidth = Detailed ? 8 : 6;
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

    FillTriangle(
        {Tip.x + 2, Tip.y + 2},
        {Left.x + 2, Left.y + 2},
        {RightPoint.x + 2, RightPoint.y + 2},
        Ink
    );
    FillTriangle(Tip, Left, RightPoint, {0.98f, 0.97f, 0.86f});
    FillCircle(PlayerScreen.x, PlayerScreen.y, Detailed ? 3 : 2, RouteColor);

    if (Detailed && !HoverLabel.empty() && GameplayTextRenderer.IsReady())""",
    "filled player arrow",
)

text = replace_once(
    text,
    """    DrawRect(X, Y, PanelWidth, 2, Ink);
    DrawRect(X, Y + PanelHeight - 2, PanelWidth, 2, Ink);
    DrawRect(X, Y, 2, PanelHeight, Ink);
    DrawRect(X + PanelWidth - 2, Y, 2, PanelHeight, Ink);
""",
    """    const glm::vec3 Frame{0.34f, 0.31f, 0.15f};
    DrawRect(X, Y, PanelWidth, 1, Frame);
    DrawRect(X, Y + PanelHeight - 1, PanelWidth, 1, Frame);
    DrawRect(X, Y, 1, PanelHeight, Frame);
    DrawRect(X + PanelWidth - 1, Y, 1, PanelHeight, Frame);
""",
    "map frame",
)

text = replace_once(
    text,
    """    const glm::vec3 Yellow = LevelZeroYellow();
    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 Muted{0.36f, 0.32f, 0.14f};
    const glm::vec3 ButtonHover{0.69f, 0.63f, 0.28f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Yellow);

    for (int X = 15; X < static_cast<int>(Width); X += 38)
        DrawRect(X, 0, 1, static_cast<int>(Height), {0.745f, 0.690f, 0.337f});
""",
    """    const glm::vec3 Yellow{0.815f, 0.765f, 0.420f};
    const glm::vec3 Ink{0.105f, 0.095f, 0.045f};
    const glm::vec3 Muted{0.34f, 0.31f, 0.16f};
    const glm::vec3 ButtonIdle{0.770f, 0.716f, 0.360f};
    const glm::vec3 ButtonHover{0.145f, 0.135f, 0.072f};
    const glm::vec3 ButtonHoverText{0.96f, 0.93f, 0.73f};

    DrawRect(0, 0, static_cast<int>(Width), static_cast<int>(Height), Yellow);

    for (int X = 32; X < static_cast<int>(Width); X += 64)
        DrawRect(X, 0, 1, static_cast<int>(Height), {0.775f, 0.724f, 0.370f});
""",
    "full map palette",
)

text = sub_once(
    text,
    r"    auto DrawButton = \[&\]\(const UiRect& Rect, const std::string& Label, float Hover\)\n    \{.*?\n    \};\n\n    DrawButton\(FullMapBackRect, \"<  BACK\", FullMapBackHover\);",
    """    auto DrawButton = [&](const UiRect& Rect, const std::string& Label, float Hover)
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
            Hovered ? RouteColor : glm::vec3{0.43f, 0.39f, 0.18f}
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

    DrawButton(FullMapBackRect, "\xE2\x86\x90  BACK", FullMapBackHover);""",
    "full map buttons",
)

text = replace_once(text, '        "LEVEL 0 MAP",', '        "LEVEL 0 / MAP",', "map title")
text = replace_once(
    text,
    '        "ESC BACK TO MENU   |   CLICK SET MARKER   |   DRAG PAN   |   WHEEL ZOOM   |   R RECENTER   |   G RANDOM",',
    '        "ESC BACK   /   CLICK WAYPOINT   /   DRAG PAN   /   WHEEL ZOOM   /   R RECENTER   /   G RANDOM",',
    "map footer",
)

write(map_path, text)

menu_path = "src/Rendering/InteractiveMenu.cpp"
text = read(menu_path)
text = replace_once(
    text,
    """    const int ParagraphWidth =
        std::min(ParagraphWidthMax, ContentWidth);
""",
    """    const int ParagraphWidth =
        std::max(
            180,
            std::min(
                590,
                std::max(ContentWidth - 20, 1)
            )
        );
""",
    "paragraph width",
)
text = replace_once(
    text,
    """            ParagraphWidth,
            [&](const std::string& Line)
""",
    """            std::max(ParagraphWidth - 16, 160),
            [&](const std::string& Line)
""",
    "paragraph wrap safety",
)
text = replace_once(
    text,
    """    const int ParagraphLineAdvance =
        static_cast<int>(
            std::round(
                static_cast<float>(ParagraphFont) *
                ParagraphLineHeight
            )
        );
""",
    """    const int MeasuredParagraphHeight =
        MenuTextRenderer.IsReady()
            ? MenuTextRenderer.MeasureHeight(
                "Ag",
                ParagraphFont,
                ParagraphWeight,
                0.0f
            )
            : ParagraphFont;

    const int ParagraphLineAdvance =
        std::max(
            static_cast<int>(
                std::round(
                    static_cast<float>(ParagraphFont) *
                    ParagraphLineHeight
                )
            ),
            MeasuredParagraphHeight + 5
        );
""",
    "paragraph measured line height",
)
text = sub_once(
    text,
    r"\n    DrawMenuText\(\n        \"THE LOBBY\",\n        ContentX,\n        TitleY \+ 1,\n        TitleSize,\n        TitleWeight,\n        TitleTracking,\n        \{1\.0f, 247\.0f / 255\.0f, 174\.0f / 255\.0f\},\n        0\.20f,\n        false\n    \);\n",
    "\n",
    "remove title ghost",
)
text = replace_once(
    text,
    '        const std::string Arrow = ">";',
    '        const std::string Arrow = "\\xE2\\x86\\x92";',
    "real menu arrow",
)
write(menu_path, text)

for renderer_path in [
    "src/Rendering/SmoothTextRenderer.cpp",
    "src/Rendering/GameTextRenderer.cpp",
]:
    text = read(renderer_path)
    text = replace_once(
        text,
        """    std::wstring Wide;
    Wide.reserve(Text.size());

    for (unsigned char Character : Text)
        Wide.push_back(static_cast<wchar_t>(Character));
""",
        """    std::wstring Wide;
    const int WideLength = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        Text.c_str(),
        static_cast<int>(Text.size()),
        nullptr,
        0
    );

    if (WideLength > 0)
    {
        Wide.resize(static_cast<std::size_t>(WideLength));
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            Text.c_str(),
            static_cast<int>(Text.size()),
            Wide.data(),
            WideLength
        );
    }
    else
    {
        Wide.reserve(Text.size());
        for (unsigned char Character : Text)
            Wide.push_back(static_cast<wchar_t>(Character));
    }
""",
        f"utf8 conversion {renderer_path}",
    )
    write(renderer_path, text)

cmake = read("CMakeLists.txt")
cmake = replace_once(
    cmake,
    "project(BackroomsOffical VERSION 0.3.27 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.28 LANGUAGES C CXX)",
    "cmake version",
)
write("CMakeLists.txt", cmake)

version = read("src/Core/Version.h")
version = replace_once(version, 'Text = "0.3.27"', 'Text = "0.3.28"', "version header")
write("src/Core/Version.h", version)

rc = read("src/Platform/Windows/Backrooms.rc")
if "0,3,27,0" not in rc or "0.3.27" not in rc:
    raise RuntimeError("Windows resource version markers not found")
rc = rc.replace("0,3,27,0", "0,3,28,0").replace("0.3.27", "0.3.28")
write("src/Platform/Windows/Backrooms.rc", rc)

notes = (
    "V0.3.28 is a visual polish release for the native map and front menu. "
    "The Level 0 map no longer uses the patchy debug-style cell gradient or dotted breadcrumb route; "
    "it uses a flatter wallpaper texture, thinner maze lines, a continuous blue navigation route, "
    "cleaner breaker/exit/entity markers, a filled directional player marker, lighter map framing, "
    "and modern flat hover controls instead of four-sided Windows-95-style boxes. "
    "The Windows text renderers now decode UTF-8 correctly so the front-menu action uses a real right-arrow glyph "
    "and the map Back control uses a proper left arrow. The front menu also uses safer measured paragraph wrapping, "
    "extra line-height clearance, and removes the duplicate title ghost pass so text is not clipped or partially rendered."
)
write("update/release_notes.txt", notes + "\n")

print("V0.3.28 visual patch applied successfully")
