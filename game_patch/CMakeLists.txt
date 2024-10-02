file(GLOB EXPERIMENTAL_SRCS experimental/*.cpp experimental/*.h)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/purefaction/pf_secret.cpp")
    set(PF_AC_SRCS
        purefaction/pf_secret_packets.h
        purefaction/pf_secret.cpp
    )
else()
    set(PF_AC_SRCS
        purefaction/pf_ac_stubs.cpp
    )
endif()


set(SRCS
    main/main.cpp
    main/main.h
    rf/os/os.h
    rf/os/console.h
    rf/os/vtypes.h
    rf/os/array.h
    rf/os/linklist.h
    rf/os/string.h
    rf/os/timestamp.h
    rf/gr/gr.h
    rf/gr/gr_font.h
    rf/gr/gr_direct3d.h
    rf/math/vector.h
    rf/math/matrix.h
    rf/math/plane.h
    rf/math/ix.h
    rf/sound/sound.h
    rf/sound/sound_ds.h
    rf/file/file.h
    rf/file/packfile.h
    rf/player/player.h
    rf/player/player_fpgun.h
    rf/player/control_config.h
    rf/player/camera.h
    rf/ai.h
    rf/bmpman.h
    rf/character.h
    rf/cutscene.h
    rf/clutter.h
    rf/corpse.h
    rf/entity.h
    rf/event.h
    rf/gameseq.h
    rf/item.h
    rf/level.h
    rf/localize.h
    rf/misc.h
    rf/multi.h
    rf/object.h
    rf/parse.h
    rf/physics.h
    rf/trigger.h
    rf/v3d.h
    rf/vmesh.h
    rf/weapon.h
    bmpman/dds.cpp
    bmpman/bmpman.cpp
    bmpman/bmpman.h
    bmpman/fmt_conv.cpp
    bmpman/fmt_conv_templates.h
    graphics/bink.cpp
    graphics/gr_font.cpp
    graphics/gr.cpp
    graphics/gr.h
    graphics/gr_light.cpp
    graphics/legacy/gr_d3d.cpp
    graphics/legacy/gr_d3d_texture.cpp
    graphics/legacy/gr_d3d_capture.cpp
    graphics/legacy/gr_d3d_gamma.cpp
    graphics/legacy/gr_d3d_internal.h
    graphics/d3d11/gr_d3d11.cpp
    graphics/d3d11/gr_d3d11.h
    graphics/d3d11/gr_d3d11_state.cpp
    graphics/d3d11/gr_d3d11_state.h
    graphics/d3d11/gr_d3d11_texture.cpp
    graphics/d3d11/gr_d3d11_texture.h
    graphics/d3d11/gr_d3d11_dynamic_geometry.cpp
    graphics/d3d11/gr_d3d11_dynamic_geometry.h
    graphics/d3d11/gr_d3d11_context.cpp
    graphics/d3d11/gr_d3d11_context.h
    graphics/d3d11/gr_d3d11_shader.cpp
    graphics/d3d11/gr_d3d11_shader.h
    graphics/d3d11/gr_d3d11_error.cpp
    graphics/d3d11/gr_d3d11_transform.h
    graphics/d3d11/gr_d3d11_solid.cpp
    graphics/d3d11/gr_d3d11_solid.h
    graphics/d3d11/gr_d3d11_mesh.cpp
    graphics/d3d11/gr_d3d11_mesh.h
    graphics/d3d11/gr_d3d11_vertex.h
    graphics/d3d11/gr_d3d11_buffer.h
    graphics/d3d11/gr_d3d11_hooks.cpp
    input/input.h
    input/mouse.cpp
    input/key.cpp
    hud/hud_personas.cpp
    hud/hud_status.cpp
    hud/hud_weapons.cpp
    hud/hud_internal.h
    hud/hud.cpp
    hud/hud.h
    hud/multi_scoreboard.cpp
    hud/multi_scoreboard.h
    hud/multi_spectate.cpp
    hud/multi_spectate.h
    hud/multi_hud.cpp
    hud/multi_hud_chat.cpp
    hud/weapon_select.cpp
    hud/message_log.cpp
    debug/debugwinmsg.cpp
    debug/debug_cmd.cpp
    debug/debug.cpp
    debug/obj_debug.cpp
    debug/debug.h
    debug/debug_internal.h
    debug/profiler.cpp
    debug/unresponsive.cpp
    multi/multi.h
    multi/multi.cpp
    multi/kill.cpp
    multi/network.cpp
    multi/level_download.cpp
    multi/server.h
    multi/server.cpp
    multi/votes.cpp
    multi/commands.cpp
    multi/multi_tdm.cpp
    multi/faction_files.cpp
    multi/faction_files.h
    multi/multi_ban.cpp
    os/console.cpp
    os/console.h
    os/commands.cpp
    os/autocomplete.cpp
    os/frametime.cpp
    os/timer.cpp
    os/win32_console.cpp
    os/os.cpp
    os/os.h
    object/object.h
    object/object_private.h
    object/entity.cpp
    object/item.cpp
    object/event.cpp
    object/trigger.cpp
    object/weapon.cpp
    object/cutscene.cpp
    object/glare.cpp
    object/object.cpp
    object/monitor.cpp
    object/particle.cpp
    object/obj_light.cpp
    misc/character.cpp
    misc/high_fps.cpp
    misc/high_fps.h
    misc/misc.cpp
    misc/misc.h
    misc/vpackfile.cpp
    misc/vpackfile.h
    misc/save_restore.cpp
    misc/main_menu.cpp
    misc/player.cpp
    misc/player_fpgun.cpp
    misc/g_solid.cpp
    misc/camera.cpp
    misc/ui.cpp
    misc/game.cpp
    misc/dashoptions.h
    misc/dashoptions.cpp
    sound/sound.cpp
    sound/sound.h
    sound/sound_ds.cpp
    res/resources.rc
    purefaction/pf.cpp
    purefaction/pf.h
    purefaction/pf_ac.h
    purefaction/pf_packets.h
    ${PF_AC_SRCS}
    ${EXPERIMENTAL_SRCS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(DashFaction SHARED ${SRCS})
target_compile_features(DashFaction PUBLIC cxx_std_20)
set_target_properties(DashFaction PROPERTIES
    PREFIX ""
    CXX_EXTENSIONS NO
)
enable_warnings(DashFaction)
setup_debug_info(DashFaction)

target_include_directories(DashFaction PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/vendor/zlib
    ${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
    ${CMAKE_SOURCE_DIR}/vendor/unrar
    ${CMAKE_SOURCE_DIR}/vendor/d3d8
    ${CMAKE_SOURCE_DIR}/vendor/xxhash
    ${CMAKE_SOURCE_DIR}/vendor/stb
    ${CMAKE_SOURCE_DIR}/vendor/freetype/include
    ${CMAKE_SOURCE_DIR}/vendor/dds
)

if(NOT MSVC)
    # natupnp.h is not distributed with MinGW
    target_include_directories(DashFaction PRIVATE ${CMAKE_SOURCE_DIR}/vendor/natupnp)
endif()

target_compile_definitions(DashFaction PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX BUILD_DLL)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/experimental/experimental.cpp")
    target_compile_definitions(DashFaction PRIVATE HAS_EXPERIMENTAL)
endif()

target_link_libraries(DashFaction
    psapi
    wininet
    version
    shlwapi
    unrar
    unzip
    zlib
    PatchCommon
    Xlog
    Common
    CrashHandlerStub
    xxhash
    gdi32
    winmm
    iphlpapi
    ws2_32
    freetype
    stb_vorbis
)

