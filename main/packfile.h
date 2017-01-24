#pragma once

#include "rf.h"

void VfsApplyHooks(void);
void ForceFileFromPackfile(const char *pszName, const char *pszPackfile);
rf::EGameLang GetInstalledGameLang();
