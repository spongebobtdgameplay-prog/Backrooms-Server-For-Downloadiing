BACKROOMS OFFICAL V0.3.11

This is the native desktop build of Backrooms Offical Level 0.

WHAT IS INCLUDED
- SDL3 native window and relative mouse input
- Windows GameInput raw keyboard/mouse path when available
- Precision Touchpad gameplay override for W + touchpad look on supported Windows 11 systems
- OpenGL 4.1 core renderer
- GPU instanced box renderer
- Dynamic fluorescent point lights
- Distance fog
- Procedural Level 0 maze
- Walls, baseboards, columns, ceiling fixtures, floor and ceiling
- Player movement, sprint, stamina, camera bob and FOV
- Circle-vs-AABB collision
- Ray/AABB raycasting for interactions and line-of-sight
- Three breaker objective
- Powered exit
- A* entity pathfinding through the generated maze
- Entity collision
- Entity near/far form change with restored squash-grow transition
- Procedural hum, static, shift, laugh and death audio
- Smooth native menu/HUD/pause/updater/end-screen text on Windows
- Windows GUI-subsystem executable with no console window during normal play
- Windows build script
- Linux build script
- CMake dependency fetching

WINDOWS BUILD
1. Install Visual Studio 2022 with "Desktop development with C++".
2. Install CMake.
3. Run build-windows.bat.
4. First build downloads SDL3, GLM and miniaudio. GLAD is vendored in the repository.
5. The executable appears at build\Release\Backrooms Offical.exe.

CONTROLS
WASD      Move
Shift     Sprint
Mouse     Look
E         Interact
Escape    Pause / resume
M         Main menu during gameplay or from pause
Enter     Start / resume
N         New session from main menu when a session exists
R         Restart after escaping or being caught

IMPORTANT
The current Windows source is compiled by GitHub Actions before release. V0.3.11 was compiled and its executable verified successfully in the dedicated typography-restoration validation workflow before promotion to main.

The original entity-ghost.glb and entity-demon.glb assets are restored under assets/models. The native renderer uses the original model assets instead of the temporary flat-box entity whenever those models load successfully.

VERSION
0.3.11

GAME ICON
- assets/icon/Backrooms.png   Exact uploaded game icon
- assets/icon/Backrooms.ico   Windows executable icon
- assets/icon/Backrooms.icns  macOS .app icon
- src/Platform/Windows/Backrooms.rc wires the icon into the Windows executable
- CMake wires Backrooms.icns into the macOS app bundle

MACOS BUILD
1. Install Xcode Command Line Tools and CMake.
2. Run: ./build-macos.sh
3. The app bundle is created at: build-macos/Backrooms Offical.app
4. Current script targets Apple Silicon arm64.

V0.3.2 BUILD FIX
- GLAD CMake helper is loaded from glad/cmake so glad_add_library is available.
- Windows build script now falls back to the standard CMake install path when PATH is stale.

V0.3.3 BUILD STRUCTURE FIX
- GLAD is now vendored directly under third_party/glad.
- The build no longer depends on GLAD's Python generator or nested CMake helper structure.
- This removes the glad_add_library configuration failure.

V0.3.4 PRESENTATION RESTORATION
- Game/executable/window renamed to Backrooms Offical.
- Original entity GLB files restored from Backrooms-Offical.
- Original entity death/laugh audio files restored.
- Procedural Level 0 wallpaper, carpet, ceiling tiles and fixture variation restored in the native shader.
- Native HUD restored: Level 0, objective, FPS, version, prompts and sprint.
- Native end screen restored.
- Windows build now copies SDL3.dll beside the executable automatically.

V0.3.5 LEVEL 0 VISUAL PARITY
- Rebalanced ambient and hemisphere lighting to match the original web build.
- Restored the original five-nearest-fluorescent light pool behavior.
- Corrected fog from linear space so distance haze matches the original #b7b08b look.
- Matched the original wallpaper stripe scale and motif spacing more closely.
- Matched wall, carpet, ceiling, trim, fixture and fluorescent-panel material colors to the original Level 0 build.

V0.3.7 MENU AND CAMERA FIX
- Replaced the temporary dark/arcade-looking start card with the original web game's bright mono-yellow Level 0 menu direction.
- Added pause menu with Resume and Main Menu.
- Main Menu preserves the active session and offers Resume Session or New Session.
- Escape pauses/resumes; M opens Main Menu while playing.
- Mouse capture is controlled by game/menu state instead of generic window clicks.
- Camera input discards capture-transition mouse spikes and clamps abnormal relative-motion bursts.
- Native camera view uses the same Y-X-Z rotation structure as the original browser camera instead of lookAt near-vertical behavior.

V0.3.10 INPUT, TEXT AND MOTION RESTORATION
- Windows executable now uses the GUI subsystem, so playing the game no longer opens a console/Windows Terminal window.
- SDL GameInput raw keyboard/mouse handling is enabled before SDL initialization when Windows supports it.
- While gameplay owns relative mouse mode, supported Windows 11 Precision Touchpads are temporarily switched to the most-sensitive keyboard coexistence mode so holding W no longer asks Windows to suppress touchpad mouse generation. The previous touchpad setting is restored on pause/menu/exit.
- The smooth antialiased text renderer is now wired through the shared text path, replacing the leftover 3x5 arcade glyphs across the start menu, HUD, pause screen, updater and end screen on Windows.
- Fixed the duplicate ENTER ENTER LEVEL 0 label.
- Restored the original stronger walk/sprint head-bob amplitudes.
- Restored the original 0.42-second ghost/demon squash-grow shapeshift transition on the real GLB models.
- Full skeletal run/walk animation playback is still a separate native renderer task; the GLB loader currently renders the model meshes without skinning animation.

V0.3.11 HTML TYPOGRAPHY RESTORATION
- Rebuilt native text styling from the original style.css instead of approximating every text role from a single scale number.
- The original HTML did not ship custom webfont files; its CSS font-family was Arial Narrow, Helvetica Neue, Arial, sans-serif.
- Native Windows text now resolves that same font stack in order, with Segoe UI only as a final platform fallback if none of those faces are installed.
- Added ClearType-quality font rasterization, CSS-like opacity and HUD text shadows.
- Gameplay HUD now uses the original web sizes, font weights, tracking, placement and brightness.
- Restored the original thin 1-pixel crosshair and horizontal sprint label/bar layout.
- Main/pause menus now use the HTML-derived V2 renderer instead of the leftover fallback menu.
- Fixed generic smooth-text sizing from Scale*8 to the old 3x5 glyph-equivalent Scale*5.
