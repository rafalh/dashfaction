#pragma once

//#include <windows.h>
#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    /* Other */

    struct Cutscene;

    static auto& default_player_weapon = AddrAsRef<String>(0x007C7600);
    static auto& active_cutscene = AddrAsRef<Cutscene*>(0x00645320);

    static auto& CutsceneIsActive = AddrAsRef<bool()>(0x0045BE80);
    static auto& GetFileChecksum = AddrAsRef<int(const char* filename)>(0x00436630);

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

    static auto& can_save = AddrAsRef<bool()>(0x004B61A0);
}
