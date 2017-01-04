#pragma once

#include "rf.h"

void VfsApplyHooks(void);
void ForceFileFromPackfile(const char *pszName, const char *pszPackfile);
EGameLang GetInstalledGameLang();
