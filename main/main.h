#pragma once

#include "rf.h"
#include "GameConfig.h"
#include "utils.h"

void FindPlayer(const StringMatcher &Query, std::function<void(rf::Player*)> Consumer);

extern GameConfig g_gameConfig;
extern HMODULE g_hModule;
