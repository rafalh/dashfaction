#pragma once

#include <windows.h>
#include <dbghelp.h>
#include <vector>
#include <string>

typedef decltype(&MiniDumpWriteDump) MiniDumpWriteDump_Type;

class MiniDumpHelper
{
private:
    HMODULE m_hDbgHelpLib = NULL;
    MiniDumpWriteDump_Type m_pMiniDumpWriteDump = NULL;
    std::vector<std::wstring> m_knownModules;
    int m_infoLevel = 0;

public:
    MiniDumpHelper();
    ~MiniDumpHelper();
    bool writeDump(const char *Path, PEXCEPTION_POINTERS pExceptionPointers, HANDLE hProcess, DWORD dwThreadId);

    // 0 - minimal, 2 - maximal
    void setInfoLevel(int infoLevel)
    {
        m_infoLevel = infoLevel;
    }

    void addKnownModule(const wchar_t *name)
    {
        m_knownModules.push_back(name);
    }

private:
    bool IsDataSectionNeeded(const WCHAR* pModuleName);
    static BOOL CALLBACK MiniDumpCallback(PVOID pParam, const PMINIDUMP_CALLBACK_INPUT pInput, PMINIDUMP_CALLBACK_OUTPUT pOutput);
};
