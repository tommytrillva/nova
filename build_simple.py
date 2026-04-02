#!/usr/bin/env python3
"""
Simple build script for VOIDSYNTH Windows VST3
"""

import os
import subprocess
import sys

def run_command(cmd, cwd=None):
    """Run a command and return success status"""
    try:
        result = subprocess.run(cmd, shell=True, check=True, cwd=cwd,
                              capture_output=True, text=True)
        print(f"[OK] {cmd}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"[FAIL] {cmd}")
        print(f"Error: {e.stderr}")
        return False

def main():
    # Change to project directory
    project_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(project_dir)

    print("Building VOIDSYNTH Windows VST3...")

    # Try different build approaches
    build_dir = "build-windows"

    # Clean build directory
    if os.path.exists(build_dir):
        print("Cleaning existing build directory...")
        subprocess.run(f'rmdir /s /q "{build_dir}"', shell=True, check=False)

    os.makedirs(build_dir, exist_ok=True)

    # Try CMake with different generators
    generators = [
        "Visual Studio 17 2022",
        "Visual Studio 16 2019",
        "NMake Makefiles",
        "MinGW Makefiles",
        "Ninja"
    ]

    for generator in generators:
        print(f"\nTrying generator: {generator}")

        # Try to configure
        config_cmd = f'cmake .. -G "{generator}"'
        if "Visual Studio" in generator:
            config_cmd += " -A x64"

        if run_command(config_cmd, build_dir):
            print(f"Configuration successful with {generator}")

            # Try to build
            build_cmd = "cmake --build . --config Release"
            if run_command(build_cmd, build_dir):
                print(f"Build successful with {generator}!")
                return True
            else:
                print(f"Build failed with {generator}")
        else:
            print(f"Configuration failed with {generator}")

    print("\nAll build attempts failed. Consider using JUCE Projucer instead.")
    return False

if __name__ == "__main__":
    success = main()
    if not success:
        sys.exit(1)