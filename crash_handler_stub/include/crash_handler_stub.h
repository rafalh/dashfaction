#pragma once

#include <windows.h>

void CrashHandlerStubInstall(HMODULE module_handle);
void CrashHandlerStubUninstall();
void CrashHandlerStubProcessException(PEXCEPTION_POINTERS exception_ptrs, DWORD thread_id);
