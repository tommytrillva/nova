# VOIDSYNTH Build Troubleshooting

## Issue: "No CMAKE_CXX_COMPILER could be found"

This means Visual Studio C++ compiler isn't properly configured. Here are solutions:

### Solution 1: Install Missing VS Components
1. Open **Visual Studio Installer**
2. Click **Modify** on Visual Studio 2022 Community
3. Ensure these are installed:
   - ✅ **Desktop development with C++**
   - ✅ **MSVC v143 - VS 2022 C++ x64/x86 build tools**
   - ✅ **Windows 10/11 SDK**

### Solution 2: Try Alternative Build Scripts
```cmd
# Try the fixed batch script
build_windows_fixed.bat

# Or try PowerShell version
powershell -ExecutionPolicy Bypass -File build_windows.ps1
```

### Solution 3: Manual Visual Studio Project
1. Open **VoidSynth.jucer** in JUCE Projucer
2. Click **"Save Project and Open in IDE"**
3. Build directly in Visual Studio

### Solution 4: Check VS Installation
```cmd
# Check if compiler exists
dir "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\"

# Should show version folder like "14.44.35207"
```

### Solution 5: Alternative Generators
If Visual Studio generator fails, try:
```cmd
# MinGW (if installed)
cmake .. -G "MinGW Makefiles"

# Or download and install MinGW-w64
```

### Solution 6: Online Build Services
- Use **GitHub Actions** for automated Windows builds
- Use **AppVeyor** or **Azure Pipelines**

## Quick Fix Commands

```cmd
# Verify VS components
"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vs_installer.exe" modify --installPath "C:\Program Files\Microsoft Visual Studio\2022\Community" --add Microsoft.VisualStudio.Workload.NativeDesktop

# Force CMake to use specific compiler
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_CXX_COMPILER="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe"
```

## Success Indicators
✅ Compiler found: `cl.exe` responds with version info
✅ CMake configures without errors
✅ Build produces `VOIDSYNTH.vst3` in output directory

## FL Studio Installation
Once built successfully:
1. Copy `VOIDSYNTH.vst3` to FL Studio plugin directory:
   - `C:\Program Files\Image-Line\FL Studio 21\Plugins\VST3\`
2. Restart FL Studio
3. Rescan plugins: **Options → File Settings → Refresh**
4. Load: **Add → Instrument → VOIDSYNTH**