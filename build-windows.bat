@echo off
setlocal

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake is not installed or is not in PATH.
    pause
    exit /b 1
)

cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    pause
    exit /b 1
)

cmake --build build --config Release
if errorlevel 1 (
    pause
    exit /b 1
)

echo.
echo Build finished.
echo EXE: build\Release\BackroomsNative.exe
pause
