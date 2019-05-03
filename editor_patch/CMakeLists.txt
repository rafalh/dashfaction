include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${CMAKE_SOURCE_DIR}/logger/include
    ${CMAKE_SOURCE_DIR}/patch_common/include
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)
add_definitions(-DNOMINMAX -DBUILD_DLL -DSUBHOOK_STATIC)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 0)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 0)

file(GLOB SRCS *.cpp *.h)
add_library(DashEditor SHARED ${SRCS})
set_property(TARGET DashEditor PROPERTY CXX_STANDARD 17)
set_property(TARGET DashEditor PROPERTY PREFIX "")

target_link_libraries(DashEditor
    PatchCommon
    Logger
    subhook
    shlwapi
)

if(MSVC)
    # generate PDB files
    set_property(TARGET DashEditor PROPERTY LINK_FLAGS /DEBUG)
else()
    # generate Map file
    set_property(TARGET DashEditor PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashEditor.map)
endif()
