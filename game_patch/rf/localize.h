#pragma once

#include <patch_common/MemUtils.h>

namespace rf
{
    namespace strings {
        static const auto &array = addr_as_ref<char*[1000]>(0x007CBBF0);
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
        static const auto &team = array[833];
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
}
