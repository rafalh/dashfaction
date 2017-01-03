#ifndef PACKFILE_H_INCLUDED
#define PACKFILE_H_INCLUDED

#include "rf.h"

void VfsApplyHooks(void);
void ForceFileFromPackfile(const char *pszName, const char *pszPackfile);
EGameLang GetInstalledGameLang();

#endif // PACKFILE_H_INCLUDED
