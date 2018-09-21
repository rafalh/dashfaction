#pragma once

#define WIN32_LEAN_AND_MEAN

#include <cstdio>
#include <cstdlib>

#include <cstdint>

#include <windows.h>
#include <shlwapi.h>
#include <wininet.h>
#include <shellapi.h>

// Needed by MinGW
#ifndef ERROR_ELEVATION_REQUIRED
#  define ERROR_ELEVATION_REQUIRED 740
#endif
