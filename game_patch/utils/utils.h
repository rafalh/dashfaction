#pragma once

#include <xlog/xlog.h>
#include <windows.h>

const char* getDxErrorStr(HRESULT hr);
std::string getOsVersion();
std::string getRealOsVersion();
bool IsCurrentUserAdmin();
const char* GetProcessElevationType();
std::string getCpuId();
std::string getCpuBrand();

#define STDCALL __stdcall

#define ASSERT(x) assert(x)
