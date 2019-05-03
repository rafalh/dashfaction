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
