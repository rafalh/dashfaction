set(SRCS
    exports.h
    graphics.cpp
    main.cpp
    trigger.cpp
    vtypes.h
    resources.rc
    resources.h
    level.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(DashEditor SHARED ${SRCS})
target_compile_features(DashEditor PUBLIC cxx_std_20)
set_target_properties(DashEditor PROPERTIES
    PREFIX ""
    CXX_EXTENSIONS NO
)
enable_warnings(DashEditor)
setup_debug_info(DashEditor)

target_compile_definitions(DashEditor PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX BUILD_DLL)

target_include_directories(DashEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

target_link_libraries(DashEditor
    PatchCommon
    Common
    Xlog
    CrashHandlerStub
    shlwapi
)
