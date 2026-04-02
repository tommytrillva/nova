@echo off
echo Building VOIDSYNTH for Windows...

REM Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Create build directory
if not exist build-windows mkdir build-windows
cd build-windows

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the project
if %ERRORLEVEL% EQU 0 (
    cmake --build . --config Release
    echo Build completed!
) else (
    echo Configuration failed!
)

pause