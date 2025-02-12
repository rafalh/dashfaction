#include "pf_ac.h"
#include "pf_packets.h"
#include "../rf/player/player.h"
#include "../rf/multi.h"

bool pf_ac_process_packet(const void*, size_t, const rf::NetAddr&, rf::Player*)
{
    return false;
}

void pf_ac_init_player(rf::Player*)
{
}

void pf_ac_verify_player(rf::Player* const player)
{
    pf_player_verified(player, pf_pure_status::none);
}

pf_pure_status pf_ac_get_pure_status(rf::Player*)
{
    return pf_pure_status::none;
}
