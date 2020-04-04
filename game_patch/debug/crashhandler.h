#pragma once

#include <windows.h>

void CrashHandlerInit(HMODULE module_handle);
void CrashHandlerCleanup();
void CrashHandlerProcessException(PEXCEPTION_POINTERS exception_ptrs, DWORD thread_id);

// Custom exception codes
namespace custom_exceptions
{
    constexpr DWORD abort             = 0xE0000001;
    constexpr DWORD invalid_parameter = 0xE0000002;
    constexpr DWORD unresponsive      = 0xE0000003;
}

