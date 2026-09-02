THE BACKROOMS NATIVE V0.3.1

This package is the native C++ rewrite of the previous browser Level 0 prototype.

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
5. The executable appears at build\Release\BackroomsNative.exe.

CONTROLS
WASD      Move
Shift     Sprint
Mouse     Look
E         Interact
Escape    Release/capture mouse
R         Restart after escaping or being caught

IMPORTANT
The source package is complete enough to build the current native prototype, but it has not been compiled inside ChatGPT's runtime because the external C++ dependencies are not installed here.

The old GLB entity models are not embedded in this ZIP. The native build currently renders the entity procedurally. That keeps the package self-contained while the native model loader is added later.

VERSION
0.3.1 Native


GAME ICON
- assets/icon/Backrooms.png   Exact uploaded game icon
- assets/icon/Backrooms.ico   Windows executable icon
- assets/icon/Backrooms.icns  macOS .app icon
- src/Platform/Windows/Backrooms.rc wires the icon into the Windows executable
- CMake wires Backrooms.icns into the macOS app bundle

MACOS BUILD
1. Install Xcode Command Line Tools and CMake.
2. Run: ./build-macos.sh
3. The app bundle is created at: build-macos/BackroomsNative.app
4. Current script targets Apple Silicon arm64.
