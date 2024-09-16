#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <common/DynamicLinkLibrary.h>

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
    DynamicLinkLibrary m_dbghelp_lib;
    MiniDumpWriteDump_Type m_MiniDumpWriteDump = nullptr;
    std::vector<std::wstring> m_known_modules;
    int m_info_level = 0;

public:
    MiniDumpHelper();
    ~MiniDumpHelper();
    bool write_dump(const char* path, PEXCEPTION_POINTERS exception_pointers, HANDLE process, DWORD thread_id);

    // 0 - minimal, 2 - maximal
    void set_info_level(int info_level)
    {
        m_info_level = info_level;
    }

    void add_known_module(const wchar_t* name)
    {
        m_known_modules.push_back(name);
    }

private:
    bool is_data_section_needed(const WCHAR* module_name);
    static BOOL CALLBACK mini_dump_callback(PVOID param, const PMINIDUMP_CALLBACK_INPUT input,
                                            PMINIDUMP_CALLBACK_OUTPUT output);
};
