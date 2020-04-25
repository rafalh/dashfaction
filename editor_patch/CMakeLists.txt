include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)
add_definitions(-DNOMINMAX -DBUILD_DLL)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

set(SRCS
    exports.h
    graphics.cpp
    main.cpp
    resources.rc
)
add_library(DashEditor SHARED ${SRCS})

target_compile_features(DashEditor PUBLIC cxx_std_17)
set_target_properties(DashEditor PROPERTIES CXX_EXTENSIONS NO)

set_property(TARGET DashEditor PROPERTY PREFIX "")

target_link_libraries(DashEditor
    PatchCommon
    Common
    Xlog
    CrashHandlerStub
    shlwapi
)

setup_debug_info(DashEditor)
