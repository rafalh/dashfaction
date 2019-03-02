file(GLOB SRCS *.c *.h)
add_library(xxhash STATIC ${SRCS})
