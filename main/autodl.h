#pragma once

#include <windows.h>

BOOL TryToDownloadLevel(const char *pszFileName);
void RenderDownloadProgress(void);
void InitAutodownloader(void);
