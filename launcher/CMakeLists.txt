include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/launcher_common
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

if(MSVC)
    set(CMAKE_MFC_FLAG 1)
endif()

file(GLOB SRCS *.cpp *.h *.rc res/*.manifest)
add_executable(DashFactionLauncher WIN32 ${SRCS})

target_link_libraries(DashFactionLauncher
    LauncherCommon
    Common
)

if(MSVC)
    # generate PDB files
    set_property(TARGET DashFactionLauncher PROPERTY LINK_FLAGS /DEBUG)
else()
    set_property(TARGET DashFactionLauncher PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashFaction.map)
endif()

