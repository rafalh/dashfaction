Building DashFaction
====================

CMake
-----

On Windows use CMake GUI to generate project files for your favorite IDE.

To cross-compile on Linux you need to specify toolchain file.
This repository contains such a file for MinGW on Ubuntu.
To generate Makefiles run:

    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-ubuntu.cmake -DCMAKE_BUILD_TYPE=Debug
