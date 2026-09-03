BACKROOMS OFFICAL V0.3.4

This is the native desktop build of Backrooms Offical Level 0.

WHAT IS INCLUDED
- SDL3 native window and relative mouse input
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
- Entity near/far form change
- Procedural hum, static, shift and death audio
- Windows build script
- Linux build script
- CMake dependency fetching

WINDOWS BUILD
1. Install Visual Studio 2022 with "Desktop development with C++".
2. Install CMake.
3. Run build-windows.bat.
4. First build downloads SDL3, GLM, GLAD and miniaudio.
5. The executable appears at build\Release\Backrooms Offical.exe.

CONTROLS
WASD      Move
Shift     Sprint
Mouse     Look
E         Interact
Escape    Release/capture mouse
R         Restart after escaping or being caught

IMPORTANT
The source package is complete enough to build the current native prototype, but it has not been compiled inside ChatGPT's runtime because the external C++ dependencies are not installed here.

The original entity-ghost.glb and entity-demon.glb assets are restored under assets/models. The renderer restoration is bringing the native build back to the original Level 0 presentation instead of the temporary flat-box look.

VERSION
0.3.4


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
