#pragma once

#include "rf.h"
#include "utils.h"

void VfsApplyHooks(void);
void ForceFileFromPackfile(const char *pszName, const char *pszPackfile);
rf::EGameLang GetInstalledGameLang();
bool IsModdedGame();
void PackfileFindMatchingFiles(const StringMatcher &Query, std::function<void(const char *)> ResultConsumer);
