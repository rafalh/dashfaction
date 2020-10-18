#pragma once

#include <functional>
#include "../utils/string-utils.h"

enum GameLang
{
    LANG_EN = 0,
    LANG_GR = 1,
    LANG_FR = 2,
};

void VPackfileApplyPatches();
GameLang GetInstalledGameLang();
bool IsModdedGame();
void VPackfileFindMatchingFiles(const StringMatcher& query, std::function<void(const char*)> result_consumer);
void VPackfileDisableOverriding();
