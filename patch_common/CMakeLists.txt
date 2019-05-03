include_directories(
    include
    ${CMAKE_SOURCE_DIR}/vendor/subhook
    ${CMAKE_SOURCE_DIR}/logger/include
)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h)

add_library(PatchCommon STATIC ${SRCS})
target_compile_features(LauncherCommon PUBLIC cxx_std_17)
set_target_properties(LauncherCommon PROPERTIES CXX_EXTENSIONS NO)
