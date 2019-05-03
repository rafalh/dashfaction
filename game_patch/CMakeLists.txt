include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
	${CMAKE_SOURCE_DIR}/logger/include
    ${CMAKE_SOURCE_DIR}/patch_common/include
	${CMAKE_SOURCE_DIR}/vendor
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
	${CMAKE_SOURCE_DIR}/vendor/unrar
	${CMAKE_SOURCE_DIR}/vendor/d3d8
	${CMAKE_SOURCE_DIR}/vendor/subhook
	${CMAKE_SOURCE_DIR}/vendor/xxhash
)

# include_directories(
#     /usr/lib/gcc/i686-w64-mingw32/7.3-posix/include/c++
#     /usr/lib/gcc/i686-w64-mingw32/7.3-posix/include/c++/i686-w64-mingw32
#     /usr/lib/gcc/i686-w64-mingw32/7.3-posix/include/c++/backward
#     /usr/lib/gcc/i686-w64-mingw32/7.3-posix/include
#     /usr/lib/gcc/i686-w64-mingw32/7.3-posix/include-fixed
#     /usr/i686-w64-mingw32/include
#     /usr/share/mingw-w64/include
# )

add_definitions(-DNOMINMAX -DBUILD_DLL -DSUBHOOK_STATIC)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pf.cpp")
    add_definitions(-DHAS_PF)
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experimental.cpp")
    add_definitions(-DHAS_EXPERIMENTAL)
endif()

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
    add_compile_options(-Wno-pmf-conversions)
endif()

set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 0)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 0)

file(GLOB SRCS *.cpp *.h *.rc)
add_library(DashFaction SHARED ${SRCS})

target_compile_features(DashFaction PUBLIC cxx_std_17)
set_target_properties(DashFaction PROPERTIES CXX_EXTENSIONS NO)

set_property(TARGET DashFaction PROPERTY PREFIX "")
#set_property(TARGET DashFaction PROPERTY CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
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
    subhook
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
