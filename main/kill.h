#pragma once

#include "rf.h"

struct PlayerStatsNew : rf::SPlayerStats
{
    unsigned short cKills, cDeaths;
};

void InitKill();
void KillInitPlayer(rf::CPlayer *pPlayer);
