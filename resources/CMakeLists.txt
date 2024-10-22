if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(VPP_TOOL ${CMAKE_SOURCE_DIR}/tools/vpp.exe)
else()
    set(VPP_TOOL ${CMAKE_SOURCE_DIR}/tools/vpp)
endif()

function(add_packfile packfile_name)

    list(TRANSFORM ARGN REPLACE "([^:]*):.*" "\\1" OUTPUT_VARIABLE input_deps)
    list(TRANSFORM ARGN REPLACE "[^:]*:(.*)" "\\1" OUTPUT_VARIABLE input_paths)

    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/bin/${packfile_name}
        COMMAND ${VPP_TOOL} -c ${CMAKE_BINARY_DIR}/bin/${packfile_name} ${input_paths}
        DEPENDS ${input_deps}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_custom_target(${packfile_name} ALL DEPENDS ${CMAKE_BINARY_DIR}/bin/${packfile_name})
endfunction()


add_packfile(dashfaction.vpp
    maps_df.txt

    images/DF_green.tga
    images/DF_red.tga
    images/scope_zoom_corner256_red.tga
    images/128blackcircle.tga
    images/ass2_scope_circle.tga
    images/DF_green_1.tga
    images/DF_red_1.tga
    images/reticle_1.tga
    images/reticle_rocket_1.tga
    images/scope_ret_1.tga
    images/ammo_bar_1.tga
    images/ammo_bar_power_1.tga
    images/ammo_signal_green_1.tga
    images/ammo_signal_red_1.tga
    images/bullet_icon_1.tga
    images/bullet_icon_50cal_1.tga
    images/bullet_icon_556_1.tga
    images/bullet_icon_aluminum_1.tga
    images/bullet_icon_gascanister_1.tga
    images/bullet_icon_powercell_1.tga
    images/bullet_icon_rocket_1.tga
    images/bullet_icon_shotgun_1.tga
    images/df_gore1.tga
    images/enviro0_1.tga
    images/enviro10_1.tga
    images/enviro20_1.tga
    images/enviro30_1.tga
    images/enviro40_1.tga
    images/enviro50_1.tga
    images/enviro60_1.tga
    images/enviro70_1.tga
    images/enviro80_1.tga
    images/enviro90_1.tga
    images/enviro100_1.tga
    images/health0_1.tga
    images/health10_1.tga
    images/health20_1.tga
    images/health30_1.tga
    images/health40_1.tga
    images/health50_1.tga
    images/health60_1.tga
    images/health70_1.tga
    images/health80_1.tga
    images/health90_1.tga
    images/health100_1.tga
    images/hud_miniflag_blue_1.vbm
    images/hud_miniflag_hilight_1.vbm
    images/hud_miniflag_red_1.vbm
    images/noclip_ammo_bar_1.tga
    images/Icon_EAX.tga
    images/mtl_archway04-mip1.tga
    images/pls_airlockmat01-mip2.tga
    images/consolebutton2A.tga

    fonts/biggerfont.vf
    fonts/regularfont.ttf
    fonts/boldfont.ttf

    meshes/Hendrix.v3c
    meshes/Generator_Small01.v3m
    meshes/Generator_Small02.v3m
    meshes/LavaTester01.v3m
    meshes/envirosuit1.v3m
    meshes/Vat1.v3m
    meshes/coffeesmokedtbl2.v3m
    meshes/coffeesmokedtblAlt.v3m
    meshes/df_meatchunks0.v3m

    standard_vs:${CMAKE_BINARY_DIR}/shaders/standard_vs.bin
    character_vs:${CMAKE_BINARY_DIR}/shaders/character_vs.bin
    transformed_vs:${CMAKE_BINARY_DIR}/shaders/transformed_vs.bin
    standard_ps:${CMAKE_BINARY_DIR}/shaders/standard_ps.bin
    ui_ps:${CMAKE_BINARY_DIR}/shaders/ui_ps.bin
)

# Copy licensing-info.txt
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/bin/licensing-info.txt
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/licensing-info.txt ${CMAKE_BINARY_DIR}/bin/licensing-info.txt
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/licensing-info.txt
)
add_custom_target(LicensingInfoTxt ALL DEPENDS ${CMAKE_BINARY_DIR}/bin/licensing-info.txt)

add_subdirectory(shaders)
