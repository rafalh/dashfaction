#pragma once

#include "rf.h"
#include "GameConfig.h"
#include "utils.h"

void FindPlayer(const StringMatcher& query, std::function<void(rf::Player*)> consumer);

extern GameConfig g_game_config;
extern HMODULE g_hmodule;
