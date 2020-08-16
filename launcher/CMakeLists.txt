include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/vendor/win32xx
)

add_definitions(-DXLOG_NO_MACROS)
enable_warnings()

file(GLOB SRCS *.cpp *.h *.rc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(DashFactionLauncher WIN32 ${SRCS})

target_compile_features(DashFactionLauncher PUBLIC cxx_std_17)
set_target_properties(DashFactionLauncher PROPERTIES CXX_EXTENSIONS NO)

target_link_libraries(DashFactionLauncher
    LauncherCommon
    Xlog
    CrashHandlerStub
    comctl32
)

setup_debug_info(DashFactionLauncher)
