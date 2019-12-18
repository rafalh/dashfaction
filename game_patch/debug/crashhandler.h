#pragma once

#include <windows.h>

void CrashHandlerInit(HMODULE module_handle);
void CrashHandlerCleanup();
