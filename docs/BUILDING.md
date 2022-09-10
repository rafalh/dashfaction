Building DashFaction
====================

Windows
-------

On Windows use CMake GUI to generate project files for your favorite IDE.
Dash Faction supports only Win32 platform. If you are using Visual Studio 2019+ on x86_64 Windows host
CMake by default selects Win64 target platform. Please change it manually to Win32.

Linux
-----

Building on Ubuntu 22.04 based distribution is recommended.
Building on other Linux distributions should be possible but may require different/additional steps that are not
covered by this instruction.

Make sure you have all the needed tools:

* mingw-w64
* cmake
* make

To install them run:

```
sudo apt-get install mingw-w64 cmake make
```

You can use Ninja instead of Make to speed up build.

Checkout source code repository:

```
git clone https://github.com/rafalh/dashfaction.git
```

Create `build` directory and generate makefiles (you can use a different directory):

```
cd dashfaction
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-ubuntu.cmake -DCMAKE_BUILD_TYPE=Release
```

Start build:

```
make -j$(nproc)
```

After build is finished you will find Dash Faction binaries in `build/Release/bin` subdirectory.

To update your custom built Dash Faction run:

```
cd dashfaction/build
git pull
make -j$(nproc)
```
