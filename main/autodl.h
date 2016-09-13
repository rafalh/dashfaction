#ifndef AUTODL_H_INCLUDED
#define AUTODL_H_INCLUDED

#include <windows.h>

BOOL TryToDownloadLevel(const char *pszFileName);
void RenderDownloadProgress(void);
void InitAutodownloader(void);

#endif // AUTODL_H_INCLUDED
