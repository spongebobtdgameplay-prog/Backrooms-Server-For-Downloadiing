@echo off
setlocal

set "CMAKE_EXE=cmake"

where cmake >nul 2>nul
if errorlevel 1 (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
    ) else (
        echo CMake was not found in PATH or at C:\Program Files\CMake\bin\cmake.exe
        pause
        exit /b 1
    )
)

"%CMAKE_EXE%" -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    pause
    exit /b 1
)

"%CMAKE_EXE%" --build build --config Release
if errorlevel 1 (
    pause
    exit /b 1
)

echo.
echo Build finished.
echo EXE: build\Release\Backrooms Offical.exe
pause
