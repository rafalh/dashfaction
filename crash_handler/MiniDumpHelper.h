#pragma once

#include <string>
#include <vector>
#include <windows.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4091)
#endif
#include <dbghelp.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

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
    bool writeDump(const char* Path, PEXCEPTION_POINTERS ExceptionPointers, HANDLE hProcess, DWORD dwThreadId);

    // 0 - minimal, 2 - maximal
    void setInfoLevel(int infoLevel)
    {
        m_infoLevel = infoLevel;
    }

    void addKnownModule(const wchar_t* name)
    {
        m_knownModules.push_back(name);
    }

private:
    bool IsDataSectionNeeded(const WCHAR* ModuleName);
    static BOOL CALLBACK MiniDumpCallback(PVOID Param, const PMINIDUMP_CALLBACK_INPUT Input,
                                          PMINIDUMP_CALLBACK_OUTPUT Output);
};
