Building DashFaction
====================

CMake
-----

On Windows use CMake GUI to generate project files for your favorite IDE.
Dash Faction supports only Win32 platform. If you are using Visual Studio 2019+ on x86_64 Windows host
CMake by default selects Win64 target platform. Please change it manually to Win32.

To cross-compile on Linux you need to specify a toolchain file.
This repository contains such a file for MinGW on Ubuntu.
To generate Makefiles run:

    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-ubuntu.cmake -DCMAKE_BUILD_TYPE=Debug
