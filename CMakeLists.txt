cmake_minimum_required(VERSION 3.8)
project(DashFaction)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cotire)

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

# Set target Windows version to XP SP3
add_definitions(
    -DWINVER=0x0501
    # needed for PROCESS_DEP_ENABLE on MinGW
    -D_WIN32_WINNT=0x0601
    -D_WIN32_IE=0x0501
    # needed for GetModuleFileNameEx resolution before Windows 7
    -DPSAPI_VERSION=1
    -DDASH_FACTION
)

include_directories(include)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")
add_compile_options("$<$<CONFIG:RELEASE>:-DNDEBUG>")

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
else()
    # Keep debugging info in Release configuration - it is stripped later
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
    # Use static linking
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static")
endif()

# Output all 'exe' and 'dll' files into 'bin' directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
    add_compile_options(/Zc:threadSafeInit-)
    add_compile_options(/arch:IA32)
    # Static linked CRT
    add_compile_options(
        "$<$<CONFIG:Debug>:/MTd>"
        "$<$<CONFIG:RelWithDebInfo>:/MT>"
        "$<$<CONFIG:Release>:/MT>"
        "$<$<CONFIG:MinSizeRel>:/MT>"
    )
endif()

# FIXME
# if(MSVC)
#     set(CMAKE_GENERATOR_TOOLSET v141_xp)
# endif()

# Macros

macro(enable_warnings)
    if(NOT MSVC)
        add_compile_options(-Wall -Wextra -Wundef)
    endif()
endmacro()

macro(setup_debug_info target)
    if(MSVC)
        # MSVC - generate PDB files
        set_property(TARGET ${target} PROPERTY LINK_FLAGS /DEBUG)
    else()
        # MinGW - copy symbols to separate file and strip from executable
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} --only-keep-debug "$<TARGET_FILE:${target}>" "$<TARGET_FILE:${target}>.debug"
            COMMAND ${CMAKE_STRIP} --strip-debug --strip-unneeded "$<TARGET_FILE:${target}>"
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="$<TARGET_FILE:${target}>.debug" "$<TARGET_FILE:${target}>")
    endif()
endmacro()

add_subdirectory(vendor)
add_subdirectory(launcher_old)
add_subdirectory(launcher_common)
add_subdirectory(launcher)
add_subdirectory(common)
add_subdirectory(game_patch)
add_subdirectory(editor_patch)
add_subdirectory(patch_common)
add_subdirectory(xlog)
add_subdirectory(crash_handler)
add_subdirectory(crash_handler_stub)
