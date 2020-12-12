include_directories(
    include/common
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

enable_warnings()

file(GLOB SRCS *.cpp include/common/*.h)

set(SRCS
    include/common/BuildConfig.h
    include/common/CfgVar.h
    include/common/ErrorUtils.h
    include/common/Exception.h
    include/common/GameConfig.h
    include/common/HttpRequest.h
    include/common/RegKey.h
    include/common/rfproto.h
    include/common/version.h
    include/common/Win32Error.h
    include/common/utils/com-utils.h
    include/common/utils/enum-bitwise-operators.h
    include/common/utils/iterable-utils.h
    include/common/utils/list-utils.h
    include/common/utils/os-utils.h
    include/common/utils/perf-utils.h
    include/common/utils/string-utils.h
    GameConfig.cpp
    HttpRequest.cpp
    utils/com-utils.cpp
    utils/os-utils.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(Common STATIC ${SRCS})

target_compile_features(Common PUBLIC cxx_std_17)
set_target_properties(Common PROPERTIES CXX_EXTENSIONS NO)

target_include_directories(Common PUBLIC include)

target_link_libraries(Common
    shlwapi
    wininet
    version
    Xlog
)
