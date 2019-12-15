if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(PatchCommon STATIC ${SRCS})
target_compile_features(PatchCommon PUBLIC cxx_std_17)
set_target_properties(PatchCommon PROPERTIES CXX_EXTENSIONS NO)

target_include_directories(PatchCommon PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/vendor/subhook
    ${CMAKE_SOURCE_DIR}/logger/include
)

target_compile_definitions(PatchCommon PUBLIC SUBHOOK_STATIC)

target_link_libraries(PatchCommon subhook Xlog)
