set(SRCS
    main.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_executable(shader_compiler ${SRCS})

target_compile_features(shader_compiler PUBLIC cxx_std_17)
set_target_properties(shader_compiler PROPERTIES CXX_EXTENSIONS NO)
enable_warnings(shader_compiler)
setup_debug_info(shader_compiler)

if (MSVC)
    target_link_libraries(shader_compiler
        d3dcompiler
    )
else()
    # Use d3dcompiler version compatible with Win 7 when building with MinGW
    target_link_libraries(shader_compiler
        d3dcompiler_43
    )
endif()
