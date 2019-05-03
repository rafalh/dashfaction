include_directories(
    include
    include/common
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp include/common/*.h)
add_library(Common STATIC ${SRCS})

target_compile_features(Common PUBLIC cxx_std_17)
set_target_properties(Common PROPERTIES CXX_EXTENSIONS NO)

target_link_libraries(Common
    shlwapi
    wininet
)
