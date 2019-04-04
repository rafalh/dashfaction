#pragma once

#include "rf.h"
#include "utils.h"

enum GameLang
{
    LANG_EN = 0,
    LANG_GR = 1,
    LANG_FR = 2,
};

void VfsApplyHooks();
void ForceFileFromPackfile(const char* name, const char* packfile_name);
GameLang GetInstalledGameLang();
bool IsModdedGame();
void PackfileFindMatchingFiles(const StringMatcher& query, std::function<void(const char*)> result_consumer);
