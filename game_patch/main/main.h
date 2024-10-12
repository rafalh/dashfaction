#pragma once

#include <common/config/GameConfig.h>

extern GameConfig g_game_config;
#ifdef _WINDOWS_
extern HMODULE g_hmodule;
#endif

void evaluate_fullbright_meshes();
