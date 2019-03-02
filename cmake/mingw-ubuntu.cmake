# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
set(TOOLCHAIN_PREFIX i686-w64-mingw32)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc-posix)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++-posix)
SET(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH  /usr/${TOOLCHAIN_PREFIX}/)

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
