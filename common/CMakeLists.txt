file(GLOB SRCS src/*.cpp include/*.h)
add_library(Common STATIC ${SRCS})
include_directories(. include ${CMAKE_SOURCE_DIR}/vendor/d3d8)
