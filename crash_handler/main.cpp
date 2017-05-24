#include <windows.h>
#include <Psapi.h>
#include "version.h"

// Config
#define CRASHHANDLER_MSG_BOX 1

// Information Level: 0 smallest - 2 - biggest
#ifdef NDEBUG
#define CRASHHANDLER_DMP_LEVEL 1
#else
#define CRASHHANDLER_DMP_LEVEL 1
#endif

#define CRASHHANDLER_DMP_FILENAME "logs/DashFaction.dmp"
#define CRASHHANDLER_MSG "Game has crashed!\nTo help resolve the problem please send files from logs subdirectory in RedFaction directory to " PRODUCT_NAME " author."

#if CRASHHANDLER_MSG_BOX
#define CRASHHANDLER_ERR(msg) MessageBox(NULL, TEXT(msg), 0, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL)
#else
#define CRASHHANDLER_ERR(msg) do {} while (0)
#endif

#include <Dbghelp.h>

typedef typename decltype(&MiniDumpWriteDump) MiniDumpWriteDump_Type;

static HMODULE g_hDbgHelpLib = NULL;
static MiniDumpWriteDump_Type g_pMiniDumpWriteDump = NULL;

//
// This function determines whether we need data sections of the given module 
//
static inline bool IsDataSectionNeeded(const WCHAR* pModuleName)
{

    // Extract the module name 
    WCHAR szFileName[_MAX_FNAME] = L"";
    _wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

    // Compare the name with the list of known names and decide 
    if (wcsicmp(szFileName, L"RF") == 0)
    {
        return true;
    }
    else if (wcsicmp(szFileName, L"DashFaction") == 0)
    {
        return true;
    }
    else if (wcsicmp(szFileName, L"ntdll") == 0)
    {
        return true;
    }

    // Complete 
    return false;

}

BOOL CALLBACK CrashHandlerMediumDumpCallback(
    PVOID                            pParam,
    const PMINIDUMP_CALLBACK_INPUT   pInput,
    PMINIDUMP_CALLBACK_OUTPUT        pOutput
)
{
    BOOL bRet = FALSE;

    // Check parameters
    if (pInput == 0 || pOutput == 0)
        return FALSE;

    // Process the callbacks
    switch (pInput->CallbackType)
    {
    case IncludeModuleCallback:
    {
        // Include the module into the dump 
        bRet = TRUE;
    }
    break;

    case IncludeThreadCallback:
    {
        // Include the thread into the dump 
        bRet = TRUE;
    }
    break;

    case ModuleCallback:
    {
        // Are data sections available for this module ? 
        if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
        {
            // Yes, they are, but do we need them? 
            if (!IsDataSectionNeeded(pInput->Module.FullPath))
            {
                //wprintf(L"Excluding module data sections: %s \n", pInput->Module.FullPath);
                pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
            }
        }

        bRet = TRUE;
    }
    break;

    case ThreadCallback:
    {
        // Include all thread information into the minidump 
        bRet = TRUE;
    }
    break;

    case ThreadExCallback:
    {
        // Include this information 
        bRet = TRUE;
    }
    break;

    case MemoryCallback:
    {
        // We do not include any information here -> return FALSE 
        bRet = FALSE;
    }
    break;

    case CancelCallback:
        break;
    }

    return bRet;
}

static inline bool CrashHandlerWriteDump(PEXCEPTION_POINTERS pExceptionPointers, HANDLE hProcess, DWORD dwThreadId)
{
    if (!g_pMiniDumpWriteDump)
        return false;

    static HANDLE hFile = NULL;
    hFile = CreateFile(
        TEXT(CRASHHANDLER_DMP_FILENAME),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        CRASHHANDLER_ERR("Error! CreateFile failed.");
        return false;
    }

    static MINIDUMP_EXCEPTION_INFORMATION ExceptionInfo;
    ExceptionInfo.ThreadId = dwThreadId;
    ExceptionInfo.ExceptionPointers = pExceptionPointers;
    ExceptionInfo.ClientPointers = TRUE;

    static MINIDUMP_TYPE DumpType;
    static MINIDUMP_CALLBACK_INFORMATION *pCallbackInfo;

    // See http://www.debuginfo.com/articles/effminidumps2.html
#if CRASHHANDLER_DMP_LEVEL == 0

    DumpType = MiniDumpWithIndirectlyReferencedMemory;
    pCallbackInfo = NULL;

#elif CRASHHANDLER_DMP_LEVEL == 1 // medium information

    DumpType = (MINIDUMP_TYPE)(
        MiniDumpWithPrivateReadWriteMemory |
        MiniDumpIgnoreInaccessibleMemory |
        MiniDumpWithDataSegs |
        MiniDumpWithHandleData |
        MiniDumpWithFullMemoryInfo |
        MiniDumpWithThreadInfo |
        MiniDumpWithUnloadedModules);

    static MINIDUMP_CALLBACK_INFORMATION CallbackInfo;
    CallbackInfo.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)CrashHandlerMediumDumpCallback;
    CallbackInfo.CallbackParam = 0;
    pCallbackInfo = &CallbackInfo;

#else // Maximal information

    DumpType = (MINIDUMP_TYPE)(MiniDumpWithFullMemory |
        MiniDumpWithFullMemoryInfo |
        MiniDumpWithHandleData |
        MiniDumpWithThreadInfo |
        MiniDumpWithUnloadedModules |
        MiniDumpWithIndirectlyReferencedMemory);
    pCallbackInfo = NULL;

#endif

    BOOL Result = g_pMiniDumpWriteDump(hProcess, GetProcessId(hProcess), hFile, DumpType, &ExceptionInfo, NULL, pCallbackInfo);
    if (!Result)
        CRASHHANDLER_ERR("MiniDumpWriteDump failed");

    CloseHandle(hFile);
    return Result != FALSE;
}

void CrashHandlerInit(void)
{
    g_hDbgHelpLib = LoadLibraryW(L"Dbghelp.dll");
    if (g_hDbgHelpLib)
        g_pMiniDumpWriteDump = (MiniDumpWriteDump_Type)GetProcAddress(g_hDbgHelpLib, "MiniDumpWriteDump");
}

void CrashHandlerCleanup(void)
{
    FreeLibrary(g_hDbgHelpLib);
    g_pMiniDumpWriteDump = NULL;
}
#if 0
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    auto argc = __argc;
    auto argv = __argv;
#else
int main(int argc, const char *argv[])
{
#endif

    if (argc < 5)
        return -1;

    EXCEPTION_POINTERS *pExceptionPtrs = (EXCEPTION_POINTERS*)strtoull(argv[1], nullptr, 0);
    HANDLE hProcess = (HANDLE)strtoull(argv[2], nullptr, 0);
    DWORD dwThreadId = (DWORD)strtoull(argv[3], nullptr, 0);
    HANDLE hEvent = (HANDLE)strtoull(argv[4], nullptr, 0);

    CrashHandlerInit();
    CrashHandlerWriteDump(pExceptionPtrs, hProcess, dwThreadId);
    CrashHandlerCleanup();

    SetEvent(hEvent);

    CloseHandle(hProcess);
    CloseHandle(hEvent);

    MessageBox(NULL, TEXT(CRASHHANDLER_MSG), TEXT("Fatal error!"), MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);

    return 0;
}
