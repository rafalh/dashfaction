include_directories(. include ${CMAKE_SOURCE_DIR}/vendor/d3d8)

file(GLOB SRCS src/*.cpp include/*.h)
add_library(Common STATIC ${SRCS})
set_property(TARGET Common PROPERTY CXX_STANDARD 17)

target_link_libraries(Common
    shlwapi
    wininet
)
