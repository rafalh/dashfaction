
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <wxx_wincore.h>
// undefine TRACE to get rid of warnings when it is defined by logging module
#undef TRACE

#include <wininet.h>
#include <shellapi.h>

#include <string>
#include <set>
#include <vector>
#include <sstream>
#include <thread>
