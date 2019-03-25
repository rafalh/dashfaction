include_directories(${CMAKE_SOURCE_DIR}/common/include)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h *.rc)
add_executable(DashFactionLauncher_OLD ${SRCS})
target_link_libraries(DashFactionLauncher_OLD shlwapi wininet)
