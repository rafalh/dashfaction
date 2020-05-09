#pragma once

#include <common/GameConfig.h>
#include <functional>
#include <map>
//#include <windows.h>
#include "rf/common.h"

// Forward declarations
namespace rf
{
    struct Player;
}

struct PlayerNetGameSaveData
{
    rf::Vector3 pos;
    rf::Matrix3 orient;
};

struct PlayerAdditionalData
{
    bool is_browser = false;
    int last_hitsound_sent_ms = 0;
    std::map<std::string, PlayerNetGameSaveData> saves;
    rf::Vector3 last_teleport_pos;
    rf::TimerApp last_teleport_timer;
};

void FindPlayer(const StringMatcher& query, std::function<void(rf::Player*)> consumer);
PlayerAdditionalData& GetPlayerAdditionalData(rf::Player* player);

extern GameConfig g_game_config;
#ifdef _WINDOWS_
extern HMODULE g_hmodule;
#endif
