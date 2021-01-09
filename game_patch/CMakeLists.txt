file(GLOB EXPERIMENTAL_SRCS experimental/*.cpp experimental/*.h)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/purefaction/pf_secret.cpp")
    set(PF_SRCS
        purefaction/pf.cpp
        purefaction/pf.h
        purefaction/pf_packets.h
        purefaction/pf_secret_packets.h
        purefaction/pf_secret.cpp
        purefaction/pf_secret.h
    )
endif()

set(SRCS
    main.cpp
    main.h
    rf/ai.h
    rf/bmpman.h
    rf/clutter.h
    rf/common.h
    rf/console.h
    rf/corpse.h
    rf/entity.h
    rf/event.h
    rf/file.h
    rf/gameseq.h
    rf/gr.h
    rf/gr_direct3d.h
    rf/item.h
    rf/level.h
    rf/localize.h
    rf/misc.h
    rf/multi.h
    rf/object.h
    rf/os.h
    rf/parse.h
    rf/physics.h
    rf/player.h
    rf/sound.h
    rf/trigger.h
    rf/vmesh.h
    rf/weapon.h
    graphics/dds.cpp
    graphics/font.cpp
    graphics/graphics.cpp
    graphics/graphics.h
    graphics/gr_color.cpp
    graphics/gr_color.h
    graphics/gamma.cpp
    graphics/capture.cpp
    graphics/gr_texture.cpp
    graphics/bmpman.cpp
    input/input.h
    input/mouse.cpp
    input/key.cpp
    in_game_ui/chatbox.cpp
    in_game_ui/health_armor_hud.cpp
    in_game_ui/hud_internal.h
    in_game_ui/hud.cpp
    in_game_ui/hud.h
    in_game_ui/persona_msg_hud.cpp
    in_game_ui/scoreboard.cpp
    in_game_ui/scoreboard.h
    in_game_ui/spectate_mode.cpp
    in_game_ui/spectate_mode.h
    in_game_ui/team_scores_hud.cpp
    in_game_ui/weapon_cycle_hud.cpp
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
    object/event.cpp
    object/trigger.cpp
    object/weapon.cpp
    object/cutscene.cpp
    object/glare.cpp
    object/object.cpp
    object/monitor.cpp
    object/particle.cpp
    object/obj_light.cpp
    misc/high_fps.cpp
    misc/high_fps.h
    misc/misc.cpp
    misc/misc.h
    misc/vpackfile.cpp
    misc/vpackfile.h
    misc/sound.cpp
    misc/sound.h
    misc/save_restore.cpp
    misc/main_menu.cpp
    misc/player.cpp
    misc/player_fpgun.cpp
    misc/bink.cpp
    misc/g_solid.cpp
    misc/camera.cpp
    misc/ui.cpp
    res/resources.rc
    ${PF_SRCS}
    ${EXPERIMENTAL_SRCS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

add_library(DashFaction SHARED ${SRCS})
target_compile_features(DashFaction PUBLIC cxx_std_17)
set_target_properties(DashFaction PROPERTIES
    PREFIX ""
    CXX_EXTENSIONS NO
)
enable_warnings(DashFaction)
setup_debug_info(DashFaction)
#cotire(DashFaction)

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

target_compile_definitions(DashFaction PRIVATE NOMINMAX BUILD_DLL)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/purefaction/pf_secret.cpp")
    target_compile_definitions(DashFaction PRIVATE HAS_PF)
endif()
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
)

