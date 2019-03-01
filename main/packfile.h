#pragma once

#include "rf.h"
#include "utils.h"

enum GameLang {
    LANG_EN = 0,
    LANG_GR = 1,
    LANG_FR = 2,
};

void VfsApplyHooks(void);
void ForceFileFromPackfile(const char *pszName, const char *pszPackfile);
GameLang GetInstalledGameLang();
bool IsModdedGame();
void PackfileFindMatchingFiles(const StringMatcher &Query, std::function<void(const char *)> ResultConsumer);
