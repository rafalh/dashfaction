set(SRCS
    sha1.cpp
    sha1.h
)
add_library(sha1 STATIC ${SRCS})
target_include_directories(sha1 PUBLIC .)
