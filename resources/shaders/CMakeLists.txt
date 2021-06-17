
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(SHADER_COMPILER ${CMAKE_BINARY_DIR}/bin/shader_compiler.exe)
else()
    set(SHADER_COMPILER wine ${CMAKE_BINARY_DIR}/bin/shader_compiler.exe)
endif()

function(add_shader TARGET_NAME SRC_FILE TARGET_LEVEL)
    set(SHADER_OUTPUT ${CMAKE_BINARY_DIR}/shaders/${TARGET_NAME}.bin)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${SHADER_COMPILER} -t ${TARGET_LEVEL} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE} -o ${SHADER_OUTPUT}
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE}
    )
    add_custom_target(${TARGET_NAME} ALL DEPENDS ${SHADER_OUTPUT})
    add_dependencies(${TARGET_NAME} shader_compiler)
endfunction()

add_shader(default_vs default_vs.hlsl vs_4_0_level_9_3)
add_shader(default_ps default_ps.hlsl ps_4_0_level_9_3)
add_shader(character_vs character_vs.hlsl vs_4_0_level_9_3)
