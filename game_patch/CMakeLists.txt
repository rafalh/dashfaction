include_directories(
	${CMAKE_SOURCE_DIR}/vendor
	${CMAKE_SOURCE_DIR}/vendor/zlib
	${CMAKE_SOURCE_DIR}/vendor/zlib/contrib/minizip
	${CMAKE_SOURCE_DIR}/vendor/unrar
	${CMAKE_SOURCE_DIR}/vendor/d3d8
	${CMAKE_SOURCE_DIR}/vendor/xxhash
)

add_definitions(-DNOMINMAX -DBUILD_DLL)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/purefaction/pf_secret.cpp")
    add_definitions(-DHAS_PF)
    set(PF_SRCS
        purefaction/pf.cpp
        purefaction/pf.h
        purefaction/pf_packets.h
        purefaction/pf_secret_packets.h
        purefaction/pf_secret.cpp
        purefaction/pf_secret.h
    )
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/misc/experimental.cpp")
    add_definitions(-DHAS_EXPERIMENTAL)
endif()

if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wundef -Wno-multichar)
    add_compile_options(-Wno-pmf-conversions)
endif()

file(GLOB SRCS misc/experimental*.cpp misc/experimental*.h)
add_library(DashFaction SHARED
    ${SRCS}
    main.cpp
    main.h
    rf/bmpman.h
    rf/clutter.h
    rf/common.h
    rf/debug_console.h
    rf/entity.h
    rf/event.h
    rf/game_seq.h
    rf/graphics.h
    rf/item.h
    rf/misc.h
    rf/network.h
    rf/object.h
    rf/parse.h
    rf/player.h
    rf/sound.h
    rf/trigger.h
    rf/weapon.h
    stdafx.h
    graphics/graphics.cpp
    graphics/graphics.h
    graphics/gr_color.cpp
    graphics/gr_color.h
    graphics/gamma.cpp
    graphics/gamma.h
    graphics/capture.cpp
    graphics/capture.h
    graphics/texture.cpp
    server/server.h
    server/server.cpp
    server/win32_server_console.cpp
    server/votes.cpp
    server/lazyban.cpp
    server/commands.cpp
    input/input.cpp
    input/input.h
    in_game_ui/hud.cpp
    in_game_ui/hud.h
    in_game_ui/scoreboard.cpp
    in_game_ui/scoreboard.h
    in_game_ui/spectate_mode.cpp
    in_game_ui/spectate_mode.h
    in_game_ui/in_game_ui.cpp
    in_game_ui/in_game_ui.h
    debug/debugwinmsg.cpp
    debug/debug_cmd.cpp
    debug/debug.cpp
    debug/obj_debug.cpp
    debug/debug.h
    debug/debug_internal.h
    debug/profiler.cpp
    debug/unresponsive.cpp
    level_autodl/autodl.cpp
    level_autodl/autodl.h
    multi/kill.cpp
    multi/kill.h
    multi/network.cpp
    multi/network.h
    utils/utils.cpp
    utils/utils.h
    console/console.cpp
    console/console.h
    console/commands.cpp
    console/autocomplete.cpp
    misc/high_fps.cpp
    misc/high_fps.h
    misc/misc.cpp
    misc/misc.h
    misc/packfile.cpp
    misc/packfile.h
    misc/wndproc.cpp
    misc/wndproc.h
    misc/sound.cpp
    misc/sound.h
    misc/limits.cpp
    misc/trigger.cpp
    misc/cutscene.cpp
    misc/event.cpp
    misc/save_restore.cpp
    misc/weapon.cpp
    misc/main_menu.cpp
    misc/glare.cpp
    ${PF_SRCS}
    res/resources.rc
)

target_compile_features(DashFaction PUBLIC cxx_std_17)
set_target_properties(DashFaction PROPERTIES CXX_EXTENSIONS NO)

set_property(TARGET DashFaction PROPERTY PREFIX "")
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
    Xlog
    Common
    CrashHandlerStub
    xxhash
    gdi32
    winmm
    ws2_32
)

if(MSVC)
    # generate PDB files
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS /DEBUG)
else()
    # generate Map file
    set_property(TARGET DashFaction PROPERTY LINK_FLAGS -Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/DashFaction.map)
endif()
