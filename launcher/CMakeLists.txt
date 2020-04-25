include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/vendor/win32xx
)

file(GLOB SRCS *.cpp *.h *.rc)
add_executable(DashFactionLauncher WIN32 ${SRCS})

target_link_libraries(DashFactionLauncher
    LauncherCommon
    Xlog
    CrashHandlerStub
    comctl32
)

setup_debug_info(DashFactionLauncher)
