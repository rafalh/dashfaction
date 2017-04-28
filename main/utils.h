#pragma once

#include <windows.h>
#include <stdint.h>
#include <log/Logger.h>

const char *stristr(const char *haystack, const char *needle);

const char *getDxErrorStr(HRESULT hr);
std::string getOsVersion();
std::string getRealOsVersion();
bool IsUserAdmin();
const char *GetProcessElevationType();
std::string getCpuId();
std::string getCpuBrand();

#define COUNTOF(a) (sizeof(a)/sizeof((a)[0]))

#define NAKED __declspec(naked)
#define STDCALL __stdcall

#define ASSERT(x) assert(x)

template<typename T>
T clamp(T val, T min, T max)
{
    return std::min(std::max(val, min), max);
}
