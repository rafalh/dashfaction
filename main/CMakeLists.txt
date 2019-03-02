include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/mod_common
    ${CMAKE_SOURCE_DIR}/mod_common
	${CMAKE_SOURCE_DIR}/logger/include
	${CMAKE_SOURCE_DIR}/common/include
	${CMAKE_SOURCE_DIR}/vendor
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
	${CMAKE_SOURCE_DIR}/vendor/unrar
	${CMAKE_SOURCE_DIR}/vendor/d3d8
	${CMAKE_SOURCE_DIR}/vendor/subhook
	${CMAKE_SOURCE_DIR}/vendor/xxhash
)
add_definitions(-DNOMINMAX -DBUILD_DLL -DSUBHOOK_STATIC)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pf.cpp")
    add_definitions(-DHAS_PF)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experimental.cpp")
    add_definitions(-DHAS_EXPERIMENTAL)
endif()

file(GLOB SRCS *.cpp *.h *.res)
add_library(DashFaction SHARED ${SRCS})
set_property(TARGET DashFaction PROPERTY CXX_STANDARD 17)
set_property(TARGET DashFaction PROPERTY PREFIX "")

target_link_libraries(DashFaction
    psapi
    wininet
    version
    shlwapi
    unrar
    unzip
    zlib
    ModCommon
    subhook
    Logger
    Common
    xxhash
    gdi32
)

if(MSVC)
    # generate PDB files
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS /DEBUG)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashFaction.map)
endif()
