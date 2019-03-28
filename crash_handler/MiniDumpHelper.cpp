#include "MiniDumpHelper.h"

#define CRASHHANDLER_ERR(msg) MessageBox(NULL, TEXT(msg), 0, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL)

//
// This function determines whether we need data sections of the given module
//
bool MiniDumpHelper::IsDataSectionNeeded(const WCHAR* ModuleName)
{
    // Extract the module name
    WCHAR FileName[_MAX_FNAME] = L"";
    _wsplitpath(ModuleName, NULL, NULL, FileName, NULL);

    // Compare the name with the list of known names and decide
    for (auto name : m_knownModules)
    {
        if (wcsicmp(FileName, name.c_str()) == 0)
        {
            return true;
        }
    }

    // Complete
    return false;
}

BOOL CALLBACK MiniDumpHelper::MiniDumpCallback(
    PVOID                            Param,
    const PMINIDUMP_CALLBACK_INPUT   Input,
    PMINIDUMP_CALLBACK_OUTPUT        Output
)
{
    MiniDumpHelper *This = (MiniDumpHelper*)Param;

    BOOL bRet = FALSE;

    // Check parameters
    if (Input == 0 || Output == 0)
        return FALSE;

    // Process the callbacks
    switch (Input->CallbackType)
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
        if (Output->ModuleWriteFlags & ModuleWriteDataSeg)
        {
            // Yes, they are, but do we need them?
            if (!This->IsDataSectionNeeded(Input->Module.FullPath))
            {
                //wprintf(L"Excluding module data sections: %s \n", Input->Module.FullPath);
                Output->ModuleWriteFlags &= (~ModuleWriteDataSeg);
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

bool MiniDumpHelper::writeDump(const char *Path, PEXCEPTION_POINTERS ExceptionPointers, HANDLE hProcess, DWORD dwThreadId)
{
    if (!m_pMiniDumpWriteDump)
        return false;

    static HANDLE hFile = NULL;
    hFile = CreateFileA(
        Path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        char Buf[256];
        sprintf(Buf, "Error %lu! CreateFile failed when writing a Minidump.", GetLastError());
        CRASHHANDLER_ERR(Buf);
        return false;
    }

    static MINIDUMP_EXCEPTION_INFORMATION ExceptionInfo;
    ExceptionInfo.ThreadId = dwThreadId;
    ExceptionInfo.ExceptionPointers = ExceptionPointers;
    ExceptionInfo.ClientPointers = TRUE;

    static MINIDUMP_TYPE DumpType = MiniDumpNormal;
    static MINIDUMP_CALLBACK_INFORMATION *CallbackInfoPtr = NULL;

    // See http://www.debuginfo.com/articles/effminidumps2.html
    if (m_infoLevel == 0)
    {
        DumpType = MiniDumpWithIndirectlyReferencedMemory;
        CallbackInfoPtr = NULL;
    }
    else if (m_infoLevel == 1) // medium information
    {
        DumpType = (MINIDUMP_TYPE)(
            MiniDumpWithPrivateReadWriteMemory |
            MiniDumpIgnoreInaccessibleMemory |
            MiniDumpWithDataSegs |
            MiniDumpWithHandleData |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules);

        static MINIDUMP_CALLBACK_INFORMATION CallbackInfo;
        CallbackInfo.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
        CallbackInfo.CallbackParam = this;
        CallbackInfoPtr = &CallbackInfo;
    }
    else if (m_infoLevel == 2) // Maximal information
    {
        DumpType = (MINIDUMP_TYPE)(MiniDumpWithFullMemory |
            MiniDumpWithFullMemoryInfo |
            MiniDumpWithHandleData |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules |
            MiniDumpWithIndirectlyReferencedMemory);
        CallbackInfoPtr = NULL;
    }

    BOOL Result = m_pMiniDumpWriteDump(hProcess, GetProcessId(hProcess), hFile, DumpType, &ExceptionInfo, NULL, CallbackInfoPtr);
    if (!Result)
        CRASHHANDLER_ERR("MiniDumpWriteDump failed");

    CloseHandle(hFile);
    return Result != FALSE;
}

MiniDumpHelper::MiniDumpHelper()
{
    m_hDbgHelpLib = LoadLibraryW(L"Dbghelp.dll");
    if (m_hDbgHelpLib)
        m_pMiniDumpWriteDump = (MiniDumpWriteDump_Type)GetProcAddress(m_hDbgHelpLib, "MiniDumpWriteDump");
}

MiniDumpHelper::~MiniDumpHelper()
{
    FreeLibrary(m_hDbgHelpLib);
    m_pMiniDumpWriteDump = NULL;
}
