include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)
add_definitions(-DNOMINMAX -DBUILD_DLL)

enable_warnings()

set(SRCS
    exports.h
    main.cpp
    resources.rc
    resources.h
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})
add_library(LauncherGuest SHARED ${SRCS})

target_compile_features(LauncherGuest PUBLIC cxx_std_17)
set_target_properties(LauncherGuest PROPERTIES CXX_EXTENSIONS NO)

set_property(TARGET LauncherGuest PROPERTY PREFIX "")

target_link_libraries(LauncherGuest
    PatchCommon
    LauncherCommon
    Common
    Xlog
    CrashHandlerStub
    shlwapi
)

setup_debug_info(LauncherGuest)
