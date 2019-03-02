file(GLOB SRCS *.cpp *.h)
add_library(ModCommon STATIC ${SRCS})
include_directories(. ${CMAKE_SOURCE_DIR}/vendor/subhook ${CMAKE_SOURCE_DIR}/logger/include)
