#pragma once

#include "rf.h"
#include "GameConfig.h"

rf::CPlayer *FindPlayer(const char *pszName);

extern GameConfig g_gameConfig;
extern HMODULE g_hModule;
