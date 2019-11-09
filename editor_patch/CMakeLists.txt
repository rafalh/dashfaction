include_directories(
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
)
add_definitions(-DNOMINMAX -DBUILD_DLL)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB SRCS *.cpp *.h)
add_library(DashEditor SHARED ${SRCS} resources.rc)

target_compile_features(DashEditor PUBLIC cxx_std_17)
set_target_properties(DashEditor PROPERTIES CXX_EXTENSIONS NO)

set_property(TARGET DashEditor PROPERTY PREFIX "")

target_link_libraries(DashEditor
    PatchCommon
    Common
    Logger
    shlwapi
)

if(MSVC)
    # generate PDB files
    set_property(TARGET DashEditor PROPERTY LINK_FLAGS /DEBUG)
else()
    # generate Map file
    set_property(TARGET DashEditor PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashEditor.map)
endif()
