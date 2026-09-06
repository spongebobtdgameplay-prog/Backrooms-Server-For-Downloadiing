from pathlib import Path
import re


def replace_regex(path, pattern, replacement, flags=re.S):
    file = Path(path)
    text = file.read_text(encoding="utf-8")
    updated, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise RuntimeError(f"{path}: expected one regex match, got {count}")
    file.write_text(updated, encoding="utf-8", newline="\n")


def replace_once(path, old, new):
    file = Path(path)
    text = file.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one exact match, got {count}")
    file.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


map_path = "src/Rendering/MapOverlay.cpp"

replace_regex(
    map_path,
    r"\n\s*const int MarkerSafeInset = Detailed \? 0 : 10;\n\n\s*if \(\n\s*Position\.x < X \+ MarkerSafeInset \|\|\n\s*Position\.y < Y \+ MarkerSafeInset \|\|\n\s*Position\.x > X \+ PanelWidth - 1 - MarkerSafeInset \|\|\n\s*Position\.y > Y \+ PanelHeight - 1 - MarkerSafeInset\n\s*\)\n\s*\{\n\s*continue;\n\s*\}\n",
    "\n"
)

replace_regex(
    map_path,
    r"            const bool Active = Marker\.Kind == MapMarkerKind::BreakerActive;.*?            ClipRect\(\n                Position\.x - \(Detailed \? 6 : 3\),.*?                Ink\n            \);(?=\n        \}\n        else if \(Marker\.Kind == MapMarkerKind::Exit)",
    '''            const bool Active = Marker.Kind == MapMarkerKind::BreakerActive;
            const glm::vec3 Glow = Active
                ? glm::vec3{0.16f, 0.92f, 0.34f}
                : glm::vec3{1.0f, 0.13f, 0.08f};

            if (!Detailed)
            {
                FillCircle(Position.x, Position.y, 4, Ink);
                FillCircle(Position.x, Position.y, 3, Glow);
                ClipRect(Position.x - 1, Position.y - 1, 2, 2, Ink);
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
            }'''
)

replace_regex(
    map_path,
    r"            const glm::vec3 Color =\n                Marker\.Kind == MapMarkerKind::ExitPowered.*?            ClipRect\(Position\.x \+ 2, Position\.y, 2, 2, Ink\);(?=\n        \}\n        else)",
    '''            const glm::vec3 Color =
                Marker.Kind == MapMarkerKind::ExitPowered
                    ? glm::vec3{0.16f, 0.95f, 0.42f}
                    : glm::vec3{0.96f, 0.94f, 0.80f};

            if (!Detailed)
            {
                ClipRect(Position.x - 4, Position.y - 4, 9, 9, Ink);
                ClipRect(Position.x - 3, Position.y - 3, 7, 7, Color);
                ClipRect(Position.x + 1, Position.y - 2, 1, 5, Ink);
            }
            else
            {
                const int Radius = 8;
                FillCircle(Position.x, Position.y, Radius + 2, Ink);
                FillCircle(Position.x, Position.y, Radius, Color);
                ClipRect(Position.x - 1, Position.y - 3, 2, 6, Ink);
                ClipRect(Position.x + 2, Position.y, 2, 2, Ink);
            }'''
)

replace_regex(
    map_path,
    r"            const bool RedPhase = std::fmod\(Time \* 5\.0f, 1\.0f\) < 0\.58f;.*?            FillCircle\(Position\.x, Position\.y \+ 6, 1, Yellow\);(?=\n        \}\n\n        if \(Detailed\))",
    '''            const bool RedPhase = std::fmod(Time * 5.0f, 1.0f) < 0.58f;
            const glm::vec3 Threat = RedPhase
                ? glm::vec3{1.0f, 0.035f, 0.02f}
                : glm::vec3{0.50f, 0.015f, 0.012f};

            if (!Detailed)
            {
                const int Radius = 4;
                const glm::ivec2 Top{Position.x, Position.y - Radius};
                const glm::ivec2 LeftPoint{Position.x - Radius, Position.y};
                const glm::ivec2 Bottom{Position.x, Position.y + Radius};
                const glm::ivec2 RightPoint{Position.x + Radius, Position.y};
                FillTriangle(Top, LeftPoint, RightPoint, Threat);
                FillTriangle(Bottom, LeftPoint, RightPoint, Threat);
                ClipRect(Position.x, Position.y, 1, 1, Yellow);
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
            }'''
)

replace_once(map_path, "    const int WallThickness = Detailed ? 1 : 2;", "    const int WallThickness = 1;")
replace_once(map_path, "    const int ArrowLength = Detailed ? 14 : 9;", "    const int ArrowLength = Detailed ? 14 : 7;")
replace_once(map_path, "    const int ArrowWidth = Detailed ? 8 : 5;", "    const int ArrowWidth = Detailed ? 8 : 4;")
replace_once(map_path, "    const float TailLength = Detailed ? 5.0f : 3.0f;", "    const float TailLength = Detailed ? 5.0f : 2.0f;")

for path in ["CMakeLists.txt", "src/Core/Version.h"]:
    file = Path(path)
    text = file.read_text(encoding="utf-8")
    if "0.3.32" not in text:
        raise RuntimeError(f"{path}: V0.3.32 marker missing")
    file.write_text(text.replace("0.3.32", "0.3.33"), encoding="utf-8", newline="\n")

rc = Path("src/Platform/Windows/Backrooms.rc")
text = rc.read_text(encoding="utf-8")
if "0,3,32,0" not in text or "0.3.32" not in text:
    raise RuntimeError("Backrooms.rc V0.3.32 markers missing")
rc.write_text(text.replace("0,3,32,0", "0,3,33,0").replace("0.3.32", "0.3.33"), encoding="utf-8", newline="\n")

Path("update/release_notes.txt").write_text(
    "V0.3.33 cleans up the gameplay minimap presentation. Gameplay minimap markers no longer disappear when their center leaves the viewport; they are drawn normally and clipped by the minimap bounds so partially visible icons remain at the edge. Breaker, exit, entity, and player markers were redesigned into smaller, simpler shapes, and local maze walls use a thinner one-pixel stroke to reduce the chunky appearance while the camera-relative rotation remains unchanged.\n",
    encoding="utf-8",
    newline="\n"
)
