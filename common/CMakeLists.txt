file(GLOB SRCS *.cpp include/common/*.h)

set(SRCS
    include/common/HttpRequest.h
    include/common/rfproto.h
    include/common/config/BuildConfig.h
    include/common/config/CfgVar.h
    include/common/config/GameConfig.h
    include/common/config/RegKey.h
    include/common/error/error-utils.h
    include/common/error/Exception.h
    include/common/error/d3d-error.h
    include/common/error/Win32Error.h
    include/common/utils/enum-bitwise-operators.h
    include/common/utils/iterable-utils.h
    include/common/utils/list-utils.h
    include/common/utils/os-utils.h
    include/common/utils/perf-utils.h
    include/common/utils/string-utils.h
    include/common/version/version.h
    src/HttpRequest.cpp
    src/config/GameConfig.cpp
    src/error/d3d-error.cpp
    src/utils/os-utils.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(Common STATIC ${SRCS})

target_compile_features(Common PUBLIC cxx_std_17)
set_target_properties(Common PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(Common)

target_include_directories(Common PRIVATE
    include/common
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

target_include_directories(Common PUBLIC include)

target_link_libraries(Common
    shlwapi
    wininet
    version
    Xlog
)
