#pragma once

#include <string>

std::string getOsVersion();
std::string getRealOsVersion();
bool IsCurrentUserAdmin();
const char* GetProcessElevationType();
std::string getCpuId();
std::string getCpuBrand();
