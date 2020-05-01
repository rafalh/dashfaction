#pragma once

#include <functional>
#include "../utils/utils.h"

enum GameLang
{
    LANG_EN = 0,
    LANG_GR = 1,
    LANG_FR = 2,
};

void PackfileApplyPatches();
GameLang GetInstalledGameLang();
bool IsModdedGame();
void PackfileFindMatchingFiles(const StringMatcher& query, std::function<void(const char*)> result_consumer);
void PrioritizePackfileWithFile(const char* filename);
