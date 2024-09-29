set(SRCS base64.cpp)
add_library(base64 STATIC ${SRCS})
target_include_directories(base64 PUBLIC .)
