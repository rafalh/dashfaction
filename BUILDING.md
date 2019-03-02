Building DashFaction
====================

Premake
-------

Compiler with C++17 support is required e.g. Visual Studio 2017 or MinGW 7.0.

On Windows download latest Premake 5 alpha version and use it to generate project files.

For example to generate project files for Visual Studio 2017 run:

    premake5 vs2017

On Linux (tested on Ubuntu 18.04) run:

    . mingw-env.sh
    tools/premake5 gmake
    cd premake-build
    make

You can experience errors caused by missing `libgcc_s_sjlj-1.dll` and `libstdc++-6.dll` in standard system paths.
To fix it run:

    sudo ./copy-mingw-dlls.sh

CMake
-----

CMake support is currently experimental.

On Windows use CMake GUI to generate project files for your favorite IDE.

To cross-compile on Linux you need to specify toolchain file.
This repository contains such a file for MinGW on Ubuntu.
To generate Makefiles run:

    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-ubuntu.cmake -DCMAKE_BUILD_TYPE=Debug
