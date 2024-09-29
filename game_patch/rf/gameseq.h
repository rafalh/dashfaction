#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    enum GameState
    {
        GS_INIT = 0x1,
        GS_MAIN_MENU = 0x2,
        GS_EXTRAS_MENU = 0x3,
        GS_INTRO_VIDEO = 0x4,
        GS_NEW_LEVEL = 0x5,
        GS_SAVE_GAME_MENU = 0x6,
        GS_LOAD_GAME_MENU = 0x7,
        GS_QUICK_SAVE = 0x8, // unused
        GS_QUICK_RESTORE = 0x9, // unused
        GS_LEVEL_TRANSITION = 0xA,
        GS_GAMEPLAY = 0xB,
        GS_C = 0xC, // unused
        GS_END_GAME = 0xD,
        GS_OPTIONS_MENU = 0xE,
        GS_MULTI_MENU = 0xF,
        GS_HELP = 0x10,
        GS_QUITING = 0x11,
        GS_MULTI_SPLITSCREEN_GAMEPLAY = 0x12,
        GS_GAME_OVER = 0x13,
        GS_MESSAGE_LOG = 0x14,
        GS_GAME_INTRO = 0x15,
        GS_FRAMERATE_TEST_END = 0x16,
        GS_CREDITS = 0x17,
        GS_BOMB_DEFUSE = 0x18,
        GS_MULTI_LEVEL_DOWNLOAD = 0x19, // unused by the base game
        GS_1A = 0x1A, // unused
        GS_1B = 0x1B, // unused
        GS_1C = 0x1C, // unused
        GS_1D = 0x1D, // unused
        GS_MULTI_SERVER_LIST = 0x1E,
        GS_MULTI_SPLITSCREEN = 0x1F,
        GS_MULTI_CREATE_GAME = 0x20,
        GS_MULTI_GETTING_STATE_INFO = 0x21,
        GS_MULTI_LIMBO = 0x22,
        GS_NUM_STATES,
    };
    static_assert(GS_NUM_STATES == 0x23, "more states will corrupt gameseq_state_info variable");

    static auto& gameseq_set_state = addr_as_ref<void(GameState state, bool force)>(0x00434190);
    static auto& gameseq_get_state = addr_as_ref<GameState()>(0x00434200);
    static auto& gameseq_get_pending_state = addr_as_ref<GameState()>(0x00434220);
    static auto& gameseq_in_gameplay = addr_as_ref<bool()>(0x00434460);
    static auto& gameseq_push_state = addr_as_ref<void(GameState state, bool transparent, bool pause_beneath)>(0x00434410);
    static auto& gameseq_process_deferred_change = addr_as_ref<GameState()>(0x00434310);
}
