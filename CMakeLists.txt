cmake_minimum_required(VERSION 3.0)
project(DashFaction)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_DEPRECATE
        -D_USE_MATH_DEFINES
        -D_SCL_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_DEPRECATE
    )
endif()

include_directories(include)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")
add_compile_options("$<$<CONFIG:RELEASE>:-DNDEBUG>")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Strip binaries in Release configuration
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    # Use static linking
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")
endif()

# Output all 'exe' and 'dll' files into 'bin' directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/Zc:threadSafeInit-)
    add_compile_options(/arch:IA32)
endif()

# FIXME
# if(MSVC)
#     set(CMAKE_GENERATOR_TOOLSET v141_xp)
# endif()

add_subdirectory(vendor)
add_subdirectory(launcher-old)
if(CMAKE_HOST_WIN32)
    add_subdirectory(launcher)
endif()
add_subdirectory(common)
add_subdirectory(main)
add_subdirectory(editor_mod)
add_subdirectory(mod_common)
add_subdirectory(logger)
add_subdirectory(crash_handler)
