#pragma once

#include "../rf/player.h"

struct PlayerStatsNew : rf::PlayerLevelStats
{
    unsigned short num_kills, num_deaths;
};

void init_kill();
void kill_init_player(rf::Player* player);
