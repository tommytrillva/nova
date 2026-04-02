set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Static link all MinGW runtimes — no external DLL dependencies
set(CMAKE_C_FLAGS "-static-libgcc" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-static -lpthread -Wl,-subsystem,windows" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "-static -lpthread -Wl,-subsystem,windows" CACHE STRING "" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "-static -lpthread -Wl,-subsystem,windows" CACHE STRING "" FORCE)

# Disable LTO — causes issues with MinGW cross-compilation
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS OFF)
