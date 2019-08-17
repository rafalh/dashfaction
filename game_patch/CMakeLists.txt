include_directories(
	${CMAKE_SOURCE_DIR}/vendor
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
	${CMAKE_SOURCE_DIR}/vendor/unrar
	${CMAKE_SOURCE_DIR}/vendor/d3d8
	${CMAKE_SOURCE_DIR}/vendor/xxhash
)

add_definitions(-DNOMINMAX -DBUILD_DLL)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/purefaction/pf_secret.cpp")
    add_definitions(-DHAS_PF)
    set(PF_SRCS
        purefaction/pf.cpp
        purefaction/pf.h
        purefaction/pf_packets.h
        purefaction/pf_secret.cpp
        purefaction/pf_secret.h
    )
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experimental.cpp")
    add_definitions(-DHAS_EXPERIMENTAL)
endif()

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
    add_compile_options(-Wno-pmf-conversions)
endif()

file(GLOB SRCS *.cpp *.h *.rc)
add_library(DashFaction SHARED
    ${SRCS}
    graphics/graphics.cpp
    graphics/graphics.h
    graphics/gr_color.cpp
    graphics/gr_color.h
    graphics/gamma.cpp
    graphics/gamma.h
    graphics/capture.cpp
    graphics/capture.h
    server/server.cpp
    server/server.h
    input/input.cpp
    input/input.h
    ${PF_SRCS}
)

target_compile_features(DashFaction PUBLIC cxx_std_17)
set_target_properties(DashFaction PROPERTIES CXX_EXTENSIONS NO)

set_property(TARGET DashFaction PROPERTY PREFIX "")
#cotire(DashFaction)

target_link_libraries(DashFaction
    psapi
    wininet
    version
    shlwapi
    unrar
    unzip
    zlib
    PatchCommon
    Logger
    Common
    xxhash
    gdi32
)

if(MSVC)
    # generate PDB files
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS /DEBUG)
else()
    # generate Map file
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashFaction.map)
endif()
