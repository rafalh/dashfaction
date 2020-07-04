#pragma once

//#include <windows.h>
#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    class CmdLineParam
    {
        using SelfType = CmdLineParam;

        SelfType *next;
        SelfType *prev;
        const char *name;
        void *unknown;
        char *arg;
        bool is_enabled;
        bool has_arg;

    public:
        CmdLineParam(const char* name, const char* unk, bool has_arg)
        {
            auto fun_ptr = reinterpret_cast<SelfType&(__thiscall*)(SelfType & self, const char* name, const char* unk, bool has_arg)>(0x00523690);
            fun_ptr(*this, name, unk, has_arg);
        }

        bool IsEnabled() const
        {
            return is_enabled;
        }

        const char* GetArg() const
        {
            return arg;
        }
    };

    /* Chat */

    enum class ChatMsgColor
    {
        red_white = 0,
        blue_white = 1,
        red_red = 2,
        blue_blue = 3,
        white_white = 4,
        default_ = 5,
    };

    static auto& ChatPrint = AddrAsRef<void(String::Pod text, ChatMsgColor color, String::Pod prefix)>(0x004785A0);
    static auto& ChatSay = AddrAsRef<void(const char *msg, bool is_team_msg)>(0x00444150);

    /* Other */

    struct RflLightmap
    {
        uint8_t *unk;
        int w;
        int h;
        uint8_t *buf;
        int bm_handle;
        int lightmap_idx;
    };
    static_assert(sizeof(RflLightmap) == 0x18);

    static auto& level_name = AddrAsRef<String>(0x00645FDC);
    static auto& level_filename = AddrAsRef<String>(0x00645FE4);
    static auto& level_author = AddrAsRef<String>(0x00645FEC);
    static auto& default_player_weapon = AddrAsRef<String>(0x007C7600);
    static auto& active_cutscene = AddrAsRef<void*>(0x00645320);
    static auto& rfl_static_geometry = AddrAsRef<void*>(0x006460E8);

    static auto& RfBeep = AddrAsRef<void(unsigned u1, unsigned u2, unsigned u3, float volume)>(0x00505560);
    static auto& SetNextLevelFilename = AddrAsRef<void(String::Pod level_filename, String::Pod save_filename)>(0x0045E2E0);
    static auto& DemoLoadLevel = AddrAsRef<void(const char *level_filename)>(0x004CC270);
    static auto& SetCursorVisible = AddrAsRef<void(bool visible)>(0x0051E680);
    static auto& CutsceneIsActive = AddrAsRef<bool()>(0x0045BE80);
    static auto& TimerGet = AddrAsRef<int(int mult)>(0x00504AB0);
    static auto& GeomClearCache = AddrAsRef<void()>(0x004F0B90);
    static auto& GetFileChecksum = AddrAsRef<int(const char* filename)>(0x00436630);

    /* Strings Table */
    namespace strings {
        static const auto &array = AddrAsRef<char*[1000]>(0x007CBBF0);
        static const auto &player = array[675];
        static const auto &frags = array[676];
        static const auto &ping = array[677];
        static const auto &caps = array[681];
        static const auto &was_killed_by_her_own_hand = array[692];
        static const auto &was_killed_by_his_own_hand = array[693];
        static const auto &was_killed_by = array[694];
        static const auto &was_killed_mysteriously = array[695];
        static const auto &has_joined = array[708];
        static const auto &score = array[720];
        static const auto &close_combat = array[779];
        static const auto &semi_auto = array[780];
        static const auto &heavy = array[781];
        static const auto &explosive = array[782];
        static const auto &player_name = array[835];
        static const auto &exiting_game = array[884];
        static const auto &usage = array[886];
        static const auto &you_killed_yourself = array[942];
        static const auto &you_just_got_beat_down_by = array[943];
        static const auto &you_were_killed_by = array[944];
        static const auto &you_killed = array[945];
        static const auto &got_beat_down_by = array[946];
        static const auto &kicking_player = array[958];
        static const auto &banning_player = array[959];
        static const auto &deathmatch = array[974];
        static const auto &capture_the_flag = array[975];
        static const auto &team_deathmatch = array[976];
    }

    /* Save-restore */

    struct LevelSaveData
    {
        char unk[0x18708];
        uint8_t num_goal_create_events;
        uint8_t num_alarm_siren_events;
        uint8_t num_when_dead_events;
        uint8_t num_cyclic_timer_events;
        uint8_t num_make_invulnerable_events;
        uint8_t num_other_events;
        uint8_t num_emitters;
        uint8_t num_decals;
        uint8_t num_entities;
        uint8_t num_items;
        uint16_t num_clutter;
        uint8_t num_triggers;
        uint8_t num_keyframes;
        uint8_t num_push_regions;
        uint8_t num_persistent_goals;
        uint8_t num_weapons;
        uint8_t num_blood_smears;
        uint8_t num_corpse;
        uint8_t num_geomod_craters;
        uint8_t num_killed_room_ids;
        uint8_t num_dead_entities;
        uint8_t num_deleted_events;
        char field_1871F;
    };
    static_assert(sizeof(LevelSaveData) == 0x18720);

    static auto& CanSave = AddrAsRef<bool()>(0x004B61A0);
}
