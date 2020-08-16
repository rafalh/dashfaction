include_directories(
    include/common
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

enable_warnings()

file(GLOB SRCS *.cpp include/common/*.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(Common STATIC ${SRCS})

target_compile_features(Common PUBLIC cxx_std_17)
set_target_properties(Common PROPERTIES CXX_EXTENSIONS NO)

target_include_directories(Common PUBLIC include)

target_link_libraries(Common
    shlwapi
    wininet
)
