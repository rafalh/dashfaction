include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/common/include
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h *.rc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(DashFactionLauncher_OLD ${SRCS})

target_link_libraries(DashFactionLauncher_OLD LauncherCommon)

setup_debug_info(DashFactionLauncher_OLD)
