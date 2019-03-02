include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/mod_common
)
add_definitions(-DNOMINMAX -DBUILD_DLL -DSUBHOOK_STATIC)

file(GLOB SRCS *.cpp *.h)
add_library(DashEditor SHARED ${SRCS})
target_link_libraries(DashEditor ModCommon Common)
set_property(TARGET DashEditor PROPERTY CXX_STANDARD 17)
set_property(TARGET DashEditor PROPERTY PREFIX "")

if(MSVC)
    # generate PDB files
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS /DEBUG)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashFaction.map)
endif()
