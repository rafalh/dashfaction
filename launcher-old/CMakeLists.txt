include_directories(${CMAKE_SOURCE_DIR}/common/include)

file(GLOB SRCS *.cpp *.h *.rc)
add_executable(DashFactionLauncher_OLD WIN32 ${SRCS})
target_link_libraries(DashFactionLauncher_OLD shlwapi wininet)
