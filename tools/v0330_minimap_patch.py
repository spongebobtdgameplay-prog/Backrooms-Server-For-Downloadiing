from pathlib import Path


def read(path):
    return Path(path).read_text(encoding="utf-8")


def write(path, text):
    Path(path).write_text(text, encoding="utf-8", newline="\n")


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


map_path = "src/Rendering/MapOverlay.cpp"
text = read(map_path)

old_marker_bounds = '''        if (
            Position.x < X - 18 ||
            Position.y < Y - 18 ||
            Position.x > X + PanelWidth + 18 ||
            Position.y > Y + PanelHeight + 18
        )
        {
            continue;
        }
'''

new_marker_bounds = '''        const int MarkerSafeInset = Detailed ? 0 : 12;

        if (
            Position.x < X + MarkerSafeInset ||
            Position.y < Y + MarkerSafeInset ||
            Position.x > X + PanelWidth - 1 - MarkerSafeInset ||
            Position.y > Y + PanelHeight - 1 - MarkerSafeInset
        )
        {
            continue;
        }
'''

text = replace_once(text, old_marker_bounds, new_marker_bounds, "marker safe bounds")

old_minimap_layout = '''    const bool Compact = Width < 900 || Height < 620;
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
'''

new_minimap_layout = '''    const bool Compact = Width < 900 || Height < 620;
    const int MapWidth = std::clamp(
        static_cast<int>(Width * (Compact ? 0.235f : 0.145f)),
        Compact ? 184 : 210,
        Compact ? 236 : 246
    );
    const int MapHeight = std::clamp(
        static_cast<int>(Height * (Compact ? 0.185f : 0.160f)),
        Compact ? 118 : 136,
        Compact ? 158 : 154
    );
    const int Margin = std::clamp(static_cast<int>(Width * 0.015f), 16, 24);
    const int MapX = Margin;
    const int MapY = static_cast<int>(Height) - MapHeight - Margin;
    const int HeaderHeight = 22;

    DrawMapPanelV2(
        World,
        MapX,
        MapY,
        MapWidth,
        MapHeight,
        PlayerPosition,
        Compact ? 2.05f : 2.35f,
        PlayerPosition,
        PlayerForward,
        Markers,
        Waypoint,
        Route,
        false,
        Time
    );

    const glm::vec3 Ink = LevelZeroInk();
    const glm::vec3 HeaderFill{0.805f, 0.752f, 0.405f};
    const glm::vec3 Separator{0.30f, 0.275f, 0.13f};
    const bool ThreatNear = std::isfinite(ThreatDistance) && ThreatDistance < 36.0f;

    DrawRect(MapX + 1, MapY + 1, MapWidth - 2, HeaderHeight, HeaderFill);
    DrawRect(MapX + 1, MapY + HeaderHeight, MapWidth - 2, 1, Separator);
'''

text = replace_once(text, old_minimap_layout, new_minimap_layout, "minimap layout")

old_label = '''        GameplayTextRenderer.Draw(
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
'''

new_label = '''        GameplayTextRenderer.Draw(
            ThreatNear ? "THREAT NEAR" : "MAP",
            MapX + 9,
            MapY + 5,
            9,
            850,
            0.09f,
            ThreatNear ? glm::vec3{0.78f, 0.025f, 0.015f} : Ink,
            1.0f,
            true
        );

        GameplayTextRenderer.Draw(
            "M",
            MapX + MapWidth - 22,
            MapY + 5,
            9,
            800,
            0.02f,
            Ink,
            0.72f,
            true
        );

        if (Waypoint.Active)
        {
            const int FooterHeight = 21;
            DrawRect(
                MapX + 1,
                MapY + MapHeight - FooterHeight - 1,
                MapWidth - 2,
                FooterHeight,
                HeaderFill
            );
            DrawRect(
                MapX + 1,
                MapY + MapHeight - FooterHeight - 1,
                MapWidth - 2,
                1,
                Separator
            );

            std::ostringstream Distance;
            Distance << "WAYPOINT  " << std::fixed << std::setprecision(0) << Waypoint.DistanceMeters << " M";
            GameplayTextRenderer.Draw(
                Distance.str(),
                MapX + 9,
                MapY + MapHeight - 18,
                9,
                800,
                0.05f,
                Ink,
                1.0f,
                true
            );
        }
'''

text = replace_once(text, old_label, new_label, "minimap labels")
write(map_path, text)

cmake_path = "CMakeLists.txt"
cmake = read(cmake_path)
cmake = replace_once(
    cmake,
    "project(BackroomsOffical VERSION 0.3.29 LANGUAGES C CXX)",
    "project(BackroomsOffical VERSION 0.3.30 LANGUAGES C CXX)",
    "cmake version",
)
write(cmake_path, cmake)

version_path = "src/Core/Version.h"
version = read(version_path)
version = replace_once(
    version,
    'inline constexpr const char* Text = "0.3.29";',
    'inline constexpr const char* Text = "0.3.30";',
    "build version",
)
write(version_path, version)

rc_path = "src/Platform/Windows/Backrooms.rc"
rc = read(rc_path)
rc = rc.replace("FILEVERSION 0,3,29,0", "FILEVERSION 0,3,30,0")
rc = rc.replace("PRODUCTVERSION 0,3,29,0", "PRODUCTVERSION 0,3,30,0")
rc = rc.replace('VALUE "FileVersion", "0.3.29\\0"', 'VALUE "FileVersion", "0.3.30\\0"')
rc = rc.replace('VALUE "ProductVersion", "0.3.29\\0"', 'VALUE "ProductVersion", "0.3.30\\0"')
if "0.3.29" in rc or "0,3,29,0" in rc:
    raise RuntimeError("old rc version remains")
write(rc_path, rc)

notes_path = "update/release_notes.txt"
notes = (
    "V0.3.30 repairs the gameplay minimap layout. The minimap is smaller and more local, uses a dedicated header instead of drawing labels over the maze, leaves more visual space below the sprint HUD, and uses safe marker bounds so breaker and threat markers are no longer sliced by the panel edges. The map hotkey is separated into a small M hint, waypoint text receives its own footer strip, and minimap text uses the same outlined gameplay text treatment introduced in V0.3.29.\n"
)
write(notes_path, notes)
