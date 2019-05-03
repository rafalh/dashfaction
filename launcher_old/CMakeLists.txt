include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/launcher_common/include
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h *.rc)
add_executable(DashFactionLauncher_OLD ${SRCS})

target_link_libraries(DashFactionLauncher_OLD LauncherCommon)
