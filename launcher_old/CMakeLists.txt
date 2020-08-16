include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/common/include
)

enable_warnings()

file(GLOB SRCS *.cpp *.h *.rc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(DashFactionLauncher_OLD ${SRCS})

target_compile_features(DashFactionLauncher_OLD PUBLIC cxx_std_17)
set_target_properties(DashFactionLauncher_OLD PROPERTIES CXX_EXTENSIONS NO)

target_link_libraries(DashFactionLauncher_OLD LauncherCommon)

setup_debug_info(DashFactionLauncher_OLD)
