if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS src/*.cpp include/*.h)
add_library(Logger STATIC ${SRCS})
target_compile_features(Logger PUBLIC cxx_std_17)
set_target_properties(Logger PROPERTIES CXX_EXTENSIONS NO)

target_include_directories(Logger PUBLIC include)
