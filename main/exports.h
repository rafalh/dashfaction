#pragma once

#include <windows.h>

/*  To use this exported function of dll, include this header
 *  in your project.
 */

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif

struct SHARED_OPTIONS;

#ifdef __cplusplus
extern "C"
{
#endif

DWORD DLL_EXPORT Init(SHARED_OPTIONS *pOptions);

#ifdef __cplusplus
}
#endif
