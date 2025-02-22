#pragma once

#include <cstddef>
#include "pf_packets.h"

// Forward declarations
namespace rf {
    struct NetAddr;
    struct Player;
}

void pf_ac_init();
bool pf_ac_process_packet(const void* data, size_t len, const rf::NetAddr& addr, rf::Player* player);
void pf_ac_init_player(rf::Player* player);
void pf_ac_verify_player(rf::Player* player);
pf_pure_status pf_ac_get_pure_status(rf::Player* player);
bool pf_ac_check_wnd_msg(unsigned msg, long wparam, long lparam);

// callback
void pf_player_verified(rf::Player* player, pf_pure_status pure_status);
