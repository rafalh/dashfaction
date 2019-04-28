include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h)
add_library(LauncherCommon STATIC ${SRCS})
set_property(TARGET LauncherCommon PROPERTY CXX_STANDARD 17)

target_link_libraries(LauncherCommon Common)
