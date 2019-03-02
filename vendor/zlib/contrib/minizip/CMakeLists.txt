include_directories(../..)

add_library(unzip STATIC unzip.c unzip.h ioapi.c ioapi.h)
add_library(zip STATIC zip.c zip.h ioapi.c ioapi.h)
