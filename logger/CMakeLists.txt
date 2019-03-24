include_directories(include)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS src/*.cpp include/*.h)
add_library(Logger STATIC ${SRCS})
