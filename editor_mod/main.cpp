#include "version.h"
#include "exports.h"
#include <MemUtils.h>
#include <AsmOpcodes.h>

#define LAUNCHER_FILENAME "DashFactionLauncher.exe"

HMODULE g_hModule;

extern "C" DWORD DLL_EXPORT Init(void *pUnused)
{
    // Prepare command
    static char CmdBuf[512];
    GetModuleFileNameA(g_hModule, CmdBuf, sizeof(CmdBuf));
    char *Ptr = strrchr(CmdBuf, '\\');
    strcpy(Ptr, "\\" LAUNCHER_FILENAME " -level \"");

    // Change command for starting level test
    WriteMemPtr(0x00447973 + 1, CmdBuf);
    WriteMemPtr(0x00447CB9 + 1, CmdBuf);
    MessageBox(0, CmdBuf, 0, 0);

    // Zero first argument for CreateProcess call
    WriteMemUInt8(0x00447B32, ASM_NOP, 0x00447B39 - 0x00447B32);
    WriteMemUInt16(0x00447B32, ASM_XOR_EAX_EAX);
    WriteMemUInt8(0x00448024, ASM_NOP, 0x0044802B - 0x00448024);
    WriteMemUInt16(0x00448024, ASM_XOR_EAX_EAX);
    
    return 1; // success
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
    g_hModule = (HMODULE)hInstance;
    DisableThreadLibraryCalls(hInstance);
    return TRUE;
}
