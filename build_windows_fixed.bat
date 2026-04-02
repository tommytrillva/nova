@echo off
echo Building VOIDSYNTH for Windows (Fixed)...

REM Check if Visual Studio is properly installed
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo Setting up Visual Studio environment...
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

REM Verify compiler is now available
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio compiler not found!
    echo Please install "Desktop development with C++" workload in Visual Studio
    pause
    exit /b 1
)

REM Show compiler info
echo Using compiler:
cl.exe 2>&1 | findstr "Version"

REM Clean and create build directory
if exist build-windows rmdir /s /q build-windows
mkdir build-windows
cd build-windows

REM Configure with explicit compiler paths
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_C_COMPILER=cl.exe

if %errorlevel% neq 0 (
    echo Configuration failed! Trying alternative approach...

    REM Try NMake approach
    cd ..
    rmdir /s /q build-windows
    mkdir build-windows
    cd build-windows

    cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release

    if %errorlevel% neq 0 (
        echo All configuration attempts failed!
        echo Try opening the project in Visual Studio manually.
        pause
        exit /b 1
    )

    echo Building with NMake...
    nmake
) else (
    echo Building with Visual Studio...
    cmake --build . --config Release
)

if %errorlevel% equ 0 (
    echo Build completed successfully!
    echo VST3 should be in: build-windows\Release\VST3\
) else (
    echo Build failed!
)

pause