#pragma once

#include "rf.h"

struct PlayerStatsNew : rf::PlayerStats
{
    unsigned short cKills, cDeaths;
};

void InitKill();
void KillInitPlayer(rf::Player* player);
