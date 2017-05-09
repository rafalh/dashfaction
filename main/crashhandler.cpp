#include "stdafx.h"
#include "crashhandler.h"
#include "version.h"

// Config
#define CRASHHANDLER_SAVE_DMP 1
#define CRASHHANDLER_SAVE_LOG 1
#define CRASHHANDLER_USE_HEURISTIC_STACK_TRACE 1
#define CRASHHANDLER_HEURISTIC_MAX_FUN_SIZE 4096
#define CRASHHANDLER_MSG_BOX 0

// 0 smallest - 2 - biggest
#ifdef NDEBUG
#define CRASHHANDLER_DMP_LEVEL 0
#else
#define CRASHHANDLER_DMP_LEVEL 1
#endif

#define CRASHHANDLER_DMP_FILENAME "logs/DashFaction.dmp"
#define CRASHHANDLER_LOG_FILENAME "logs/DashFaction.crash.log"
#define CRASHHANDLER_MSG "Game has crashed!\nTo help resolve the problem please send files from logs subdirectory in RedFaction directory to " PRODUCT_NAME " author."

#if CRASHHANDLER_MSG_BOX
#define CRASHHANDLER_ERR(msg) MessageBox(NULL, TEXT(msg), 0, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL)
#else
#define CRASHHANDLER_ERR(msg) do {} while (0)
#endif

#ifdef _MSC_VER
#include <Dbghelp.h>
#else

typedef struct _MINIDUMP_EXCEPTION_INFORMATION
{
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef enum _MINIDUMP_TYPE
{
    MiniDumpNormal                           = 0x00000000,
    MiniDumpWithDataSegs                     = 0x00000001,
    MiniDumpWithFullMemory                   = 0x00000002,
    MiniDumpWithHandleData                   = 0x00000004,
    MiniDumpFilterMemory                     = 0x00000008,
    MiniDumpScanMemory                       = 0x00000010,
    MiniDumpWithUnloadedModules              = 0x00000020,
    MiniDumpWithIndirectlyReferencedMemory   = 0x00000040,
    MiniDumpFilterModulePaths                = 0x00000080,
    MiniDumpWithProcessThreadData            = 0x00000100,
    MiniDumpWithPrivateReadWriteMemory       = 0x00000200,
    MiniDumpWithoutOptionalData              = 0x00000400,
    MiniDumpWithFullMemoryInfo               = 0x00000800,
    MiniDumpWithThreadInfo                   = 0x00001000,
    MiniDumpWithCodeSegs                     = 0x00002000,
    MiniDumpWithoutAuxiliaryState            = 0x00004000,
    MiniDumpWithFullAuxiliaryState           = 0x00008000,
    MiniDumpWithPrivateWriteCopyMemory       = 0x00010000,
    MiniDumpIgnoreInaccessibleMemory         = 0x00020000,
    MiniDumpWithTokenInformation             = 0x00040000
} MINIDUMP_TYPE;

typedef void *PMINIDUMP_USER_STREAM_INFORMATION;
typedef void *PMINIDUMP_CALLBACK_INFORMATION;

#endif /* !_MSC_VER */

typedef BOOL (WINAPI *MiniDumpWriteDump_Type)(
    HANDLE hProcess,
    DWORD ProcessId,
    HANDLE hFile,
    MINIDUMP_TYPE DumpType,
    PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    PMINIDUMP_CALLBACK_INFORMATION CallbackParam
);

#if CRASHHANDLER_SAVE_DMP
static HMODULE g_hDbgHelpLib = NULL;
static MiniDumpWriteDump_Type g_pMiniDumpWriteDump = NULL;
#endif

#if CRASHHANDLER_USE_HEURISTIC_STACK_TRACE == 0

static inline void WriteFrameBasedStackTrace(FILE *pFile, DWORD Ebp)
{
    PVOID *pFrame = (PVOID*)Ebp;
    while (!IsBadReadPtr(pFrame, 8))
    {
        fprintf(pFile, "%08lX\n", (DWORD)pFrame[1]);
        if (pFrame == (PVOID)pFrame[0])
        {
            fprintf(g_pFile, "...\n");
            break;
        }
        pFrame = (PVOID)pFrame[0];
    }
}

#else // CRASHHANDLER_USE_HEURISTIC_STACK_TRACE

static void inline WriteHeuristicStackTrace(FILE *pFile, DWORD Eip, DWORD Esp)
{
    int i;
    BYTE *PrevCodePtr = (BYTE*)Eip;
    for (i = 0; i < 1024; ++i)
    {
        // Read next values from the stack
        DWORD *StackPtr = ((DWORD*)Esp) + i;
        if (IsBadReadPtr(StackPtr, sizeof(DWORD)))
            break;
        BYTE *CodePtr = (BYTE*)*StackPtr;

        // Check if this is address to code
        if (IsBadCodePtr((FARPROC)(CodePtr - 6)) || IsBadCodePtr((FARPROC)CodePtr))
            continue;

        if (CodePtr[-5] == 0xE8) // call rel32
        {
            BYTE *CallAddr = (BYTE*)((int)CodePtr + *(int*)(CodePtr - 4));
            //WARN("%p %p %u", CodePtr, CallAddr, abs(CallAddr - PrevCodePtr));
            if (abs(CallAddr - PrevCodePtr) > CRASHHANDLER_HEURISTIC_MAX_FUN_SIZE) // check if call destinatio is near 
                continue;
            fprintf(pFile, "%08p\n", CodePtr - 5);
        }
        //else if (CodePtr[-6] == 0xFF && CodePtr[-5] == 0x15) // call ds:rel32
        //    fprintf(g_pFile, "%08p\n", CodePtr - 6);
        else
            continue;

        PrevCodePtr = CodePtr;
    }
}

#endif // CRASHHANDLER_USE_HEURISTIC_STACK_TRACE

static int ModuleHandleCompare(const void *phMod1, const void *phMod2)
{
    return *((HMODULE*)phMod1) - *((HMODULE*)phMod2);
}

#if CRASHHANDLER_SAVE_DMP

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

static inline bool CrashHandlerWriteDump(PEXCEPTION_POINTERS pExceptionPointers)
{
    if (!g_pMiniDumpWriteDump)
        return false;

    static HANDLE hFile = NULL;
    hFile = CreateFile(
        TEXT(CRASHHANDLER_DMP_FILENAME),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
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
    ExceptionInfo.ThreadId = GetCurrentThreadId();
    ExceptionInfo.ExceptionPointers = pExceptionPointers;
    ExceptionInfo.ClientPointers = FALSE;

    static MINIDUMP_TYPE DumpType;
    static MINIDUMP_CALLBACK_INFORMATION *pCallbackInfo;

    // See http://www.debuginfo.com/articles/effminidumps2.html
#if CRASHHANDLER_DMP_LEVEL == 0

    DumpType = MiniDumpWithIndirectlyReferencedMemory;
    pCallbackInfo = NULL;

#elif CRASHHANDLER_DMP_LEVEL == 1 // medium information

    // Note: MiniDumpWithPrivateReadWriteMemory makes VS2015 unable to load dump and also makes it very huge (more than 100-200 MB for RF)
    DumpType = (MINIDUMP_TYPE)( // MiniDumpWithPrivateReadWriteMemory |
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

    BOOL Result = g_pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, DumpType, &ExceptionInfo, NULL, pCallbackInfo);
    if (!Result)
        CRASHHANDLER_ERR("MiniDumpWriteDump failed");

    CloseHandle(hFile);
    return Result != FALSE;
}

#endif // CRASHHANDLER_SAVE_DMP

#if CRASHHANDLER_SAVE_LOG

static inline bool CrashHandlerWriteLog(PEXCEPTION_POINTERS pExceptionPointers)
{
    static FILE *pFile = NULL;
    pFile = fopen(CRASHHANDLER_LOG_FILENAME, "w");
    if (!pFile)
        return false;

    time_t now = time(0);
    tm *localtm = localtime(&now);

    fprintf(pFile, "Error occurred at: %s", asctime(localtm));
    fprintf(pFile, "ExceptionCode: %08lX\n", pExceptionPointers->ExceptionRecord->ExceptionCode);
    fprintf(pFile, "ExceptionFlags: %08lX\n", pExceptionPointers->ExceptionRecord->ExceptionFlags);
    fprintf(pFile, "ExceptionAddress: %p\n", pExceptionPointers->ExceptionRecord->ExceptionAddress);

    fprintf(pFile, "ExceptionInformation:");
    for (unsigned i = 0; i < pExceptionPointers->ExceptionRecord->NumberParameters; ++i)
        fprintf(pFile, " %08lX", pExceptionPointers->ExceptionRecord->ExceptionInformation[i]);
    fprintf(pFile, "\n");

#ifdef _X86_
    fprintf(pFile, "\nRegisters:\n");
    fprintf(pFile, "EIP: %08lX\t", pExceptionPointers->ContextRecord->Eip);
    fprintf(pFile, "EBP: %08lX\t", pExceptionPointers->ContextRecord->Ebp);
    fprintf(pFile, "ESP: %08lX\n", pExceptionPointers->ContextRecord->Esp);
    fprintf(pFile, "EAX: %08lX\t", pExceptionPointers->ContextRecord->Eax);
    fprintf(pFile, "EBX: %08lX\t", pExceptionPointers->ContextRecord->Ebx);
    fprintf(pFile, "ECX: %08lX\t", pExceptionPointers->ContextRecord->Ecx);
    fprintf(pFile, "EDX: %08lX\n", pExceptionPointers->ContextRecord->Edx);
    fprintf(pFile, "EDI: %08lX\t", pExceptionPointers->ContextRecord->Edi);
    fprintf(pFile, "ESI: %08lX\n", pExceptionPointers->ContextRecord->Esi);

    // Note: Red Faction has 'omit frame pointer' optimization enabled
    fprintf(pFile, "\nStacktrace:\n%08lX\n", pExceptionPointers->ContextRecord->Eip);
#if CRASHHANDLER_USE_HEURISTIC_STACK_TRACE
    WriteHeuristicStackTrace(pFile, pExceptionPointers->ContextRecord->Eip, pExceptionPointers->ContextRecord->Esp);
#else
    WriteFrameBasedStackTrace(pFile, pExceptionPointers->ContextRecord->Ebp);
#endif

    fprintf(pFile, "\nStack:\n");
    for (unsigned i = 0; i < 1024; ++i)
    {
        DWORD *Ptr = ((DWORD*)pExceptionPointers->ContextRecord->Esp) + i;
        if (IsBadReadPtr(Ptr, sizeof(DWORD)))
            break;
        fprintf(pFile, "%08lX%s", *Ptr, i % 8 == 7 ? "\n" : " ");
    }

    static HMODULE Modules[256];
    static DWORD cModules;
    static char szBuffer[256];
    if (EnumProcessModules(GetCurrentProcess(), Modules, sizeof(Modules), &cModules))
    {
        if (cModules > sizeof(Modules))
            cModules = sizeof(Modules);
        cModules /= sizeof(HMODULE);
        qsort(Modules, cModules, sizeof(HMODULE), ModuleHandleCompare);

        fprintf(pFile, "\nModules:\n");
        for (unsigned i = 0; i < cModules; ++i)
        {
            if (!GetModuleFileName(Modules[i], szBuffer, sizeof(szBuffer)))
                szBuffer[0] = 0;
            fprintf(pFile, "%p\t%s\n", Modules[i], szBuffer);
        }
    }

#endif // _X86_
    fclose(pFile);
    return true;
}

#endif // CRASHHANDLER_SAVE_LOG

static LONG WINAPI CrashHandlerExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers)
{
#if CRASHHANDLER_SAVE_DMP
    CrashHandlerWriteDump(pExceptionPointers);
#endif // CRASHHANDLER_SAVE_DMP
#if CRASHHANDLER_SAVE_LOG
    CrashHandlerWriteLog(pExceptionPointers);
#endif // CRASHHANDLER_SAVE_LOG

#if CRASHHANDLER_MSG_BOX
    MessageBox(NULL, TEXT(CRASHHANDLER_MSG), "Fatal error!", MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL);
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}

void CrashHandlerInit(void)
{
#if CRASHHANDLER_SAVE_DMP
    g_hDbgHelpLib = LoadLibrary(TEXT("Dbghelp.dll"));
    if (g_hDbgHelpLib)
        g_pMiniDumpWriteDump = (MiniDumpWriteDump_Type)GetProcAddress(g_hDbgHelpLib, "MiniDumpWriteDump");
#endif // CRASHHANDLER_SAVE_DMP

    SetUnhandledExceptionFilter(CrashHandlerExceptionFilter);
}

void CrashHandlerCleanup(void)
{
#if CRASHHANDLER_SAVE_DMP
    FreeLibrary(g_hDbgHelpLib);
    g_pMiniDumpWriteDump = NULL;
#endif // CRASHHANDLER_SAVE_DMP
}
