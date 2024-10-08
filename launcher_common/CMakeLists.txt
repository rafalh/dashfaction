set(SRCS
    DllInjector.cpp
    DllInjector.h
    InjectingProcessLauncher.cpp
    InjectingProcessLauncher.h
    PatchedAppLauncher.cpp
    Process.h
    Thread.h
    UpdateChecker.cpp
    VideoDeviceInfoProviderD3D8.cpp
    VideoDeviceInfoProviderD3D9.cpp
    VideoDeviceInfoProviderD3D11.cpp
    Win32Handle.h
    include/launcher_common/PatchedAppLauncher.h
    include/launcher_common/UpdateChecker.h
    include/launcher_common/VideoDeviceInfoProvider.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(LauncherCommon STATIC ${SRCS})

target_compile_features(LauncherCommon PUBLIC cxx_std_20)
set_target_properties(LauncherCommon PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(LauncherCommon)

target_include_directories(LauncherCommon
    PUBLIC
    include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_CURRENT_SOURCE_DIR}/include/launcher_common
)

target_link_libraries(LauncherCommon windowscodecs Common Xlog sha1 ed25519 base64)
