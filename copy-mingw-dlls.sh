#!/bin/sh
MINGW_VER=7.3-posix
MINGW_DIR=/usr/lib/gcc/i686-w64-mingw32/$MINGW_VER
OUT_DIR=bin/debug
cp $MINGW_DIR/libgcc_s_sjlj-1.dll $OUT_DIR
cp $MINGW_DIR/libstdc++-6.dll $OUT_DIR
cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $OUT_DIR
