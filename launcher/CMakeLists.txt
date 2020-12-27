file(GLOB SRCS *.cpp *.h *.rc)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(DashFactionLauncher WIN32 ${SRCS})

target_compile_features(DashFactionLauncher PUBLIC cxx_std_17)
set_target_properties(DashFactionLauncher PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(DashFactionLauncher)
setup_debug_info(DashFactionLauncher)

target_compile_definitions(DashFactionLauncher PRIVATE XLOG_NO_MACROS)

target_include_directories(DashFactionLauncher SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/vendor/win32xx
)

target_link_libraries(DashFactionLauncher
    LauncherCommon
    Xlog
    CrashHandlerStub
    comctl32
)
