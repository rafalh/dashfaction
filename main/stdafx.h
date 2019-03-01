#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601 // needed for PROCESS_DEP_ENABLE on MinGW

#include <windows.h>
#include <psapi.h>
#include <wininet.h>
#include <winsock.h>
#include <shlwapi.h>

typedef BOOL WINBOOL;
#include <d3d8.h>

#include <unzip.h>
#include <unrar/dll.hpp>

#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <map>
#include <iomanip>

#include <MemUtils.h>
#include <AsmOpcodes.h>
#include <AsmWritter.h>

#include "BuildConfig.h"
