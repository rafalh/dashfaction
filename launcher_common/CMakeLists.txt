file(GLOB SRCS *.cpp *.h include/launcher_common/*.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(LauncherCommon STATIC ${SRCS})

target_compile_features(LauncherCommon PUBLIC cxx_std_17)
set_target_properties(LauncherCommon PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(LauncherCommon)

target_include_directories(LauncherCommon
    PUBLIC
    include
    PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_CURRENT_SOURCE_DIR}/include/launcher_common
)

target_link_libraries(LauncherCommon Common Xlog)
