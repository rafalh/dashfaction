file(GLOB SRCS src/*.cpp include/*.h)
add_library(Logger STATIC ${SRCS})
include_directories(include)
