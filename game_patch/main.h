#pragma once

#include "rf.h"
#include <common/GameConfig.h>
#include "utils.h"

struct PlayerAdditionalData
{
    bool is_browser = false;
};

void FindPlayer(const StringMatcher& query, std::function<void(rf::Player*)> consumer);
PlayerAdditionalData& GetPlayerAdditionalData(rf::Player* player);

extern GameConfig g_game_config;
extern HMODULE g_hmodule;
