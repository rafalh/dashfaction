#pragma once

#include "../rf.h"

struct PlayerStatsNew : rf::PlayerStats
{
    unsigned short num_kills, num_deaths;
};

void InitKill();
void KillInitPlayer(rf::Player* player);
