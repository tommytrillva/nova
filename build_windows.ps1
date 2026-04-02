# PowerShell Build Script for VOIDSYNTH
Write-Host "Building VOIDSYNTH with PowerShell..." -ForegroundColor Green

# Import Visual Studio environment
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if (Test-Path $vcvars) {
    Write-Host "Setting up Visual Studio environment..." -ForegroundColor Yellow
    cmd /c "`"$vcvars`" && set" | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
} else {
    Write-Host "Visual Studio not found!" -ForegroundColor Red
    exit 1
}

# Check for compiler
$clPath = Get-Command cl.exe -ErrorAction SilentlyContinue
if (-not $clPath) {
    Write-Host "Compiler not found after environment setup!" -ForegroundColor Red
    Write-Host "Please install 'Desktop development with C++' workload in Visual Studio" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found compiler: $($clPath.Source)" -ForegroundColor Green

# Clean build directory
if (Test-Path "build-windows") {
    Remove-Item "build-windows" -Recurse -Force
}
New-Item -ItemType Directory -Name "build-windows" | Out-Null
Set-Location "build-windows"

# Configure with CMake
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
& cmake .. -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -eq 0) {
    Write-Host "Building..." -ForegroundColor Yellow
    & cmake --build . --config Release

    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build completed successfully!" -ForegroundColor Green
        Write-Host "VST3 location: build-windows\Release\VST3\" -ForegroundColor Cyan
    } else {
        Write-Host "Build failed!" -ForegroundColor Red
    }
} else {
    Write-Host "Configuration failed!" -ForegroundColor Red
}

Set-Location ".."
Read-Host "Press Enter to continue"