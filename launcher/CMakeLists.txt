include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)

file(GLOB SRCS *.cpp *.h *.rc res/*.manifest)
add_executable(DashFactionLauncher WIN32 ${SRCS})
target_link_libraries(DashFactionLauncher shlwapi wininet psapi Common)

# TODO flags { "MFC", "WinMain" }

if(MSVC)
    # generate PDB files
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS /DEBUG)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashFaction.map)
endif()

