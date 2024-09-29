#include "MiniDumpHelper.h"
#include <format>

#define CRASHHANDLER_ERR(msg) MessageBox(nullptr, TEXT(msg), 0, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TASKMODAL)

//
// This function determines whether we need data sections of the given module
//
bool MiniDumpHelper::is_data_section_needed(const WCHAR* module_name)
{
    // Extract the module name
    WCHAR file_name[_MAX_FNAME] = L"";
    _wsplitpath(module_name, nullptr, nullptr, file_name, nullptr);

    // Compare the name with the list of known names and decide
    for (const auto& name : m_known_modules) {
        if (wcsicmp(file_name, name.c_str()) == 0) {
            return true;
        }
    }

    // Complete
    return false;
}

BOOL CALLBACK MiniDumpHelper::mini_dump_callback(PVOID param, const PMINIDUMP_CALLBACK_INPUT input,
                                                 PMINIDUMP_CALLBACK_OUTPUT output)
{
    auto* that = reinterpret_cast<MiniDumpHelper*>(param);

    // Check parameters
    if (!input || !output)
        return FALSE;

    // Process the callbacks
    switch (input->CallbackType) {
    case IncludeModuleCallback:
    case IncludeThreadCallback:
    case ThreadCallback:
    case ThreadExCallback:
        // Include the module into the dump
        // Include the thread into the dump
        // Include all thread information into the minidump
        return TRUE;

    case ModuleCallback:
        // Are data sections available for this module ?
        if (output->ModuleWriteFlags & ModuleWriteDataSeg) {
            // Yes, they are, but do we need them?
            if (!that->is_data_section_needed(input->Module.FullPath)) {
                // wprintf(L"Excluding module data sections: %s \n", Input->Module.FullPath);
                output->ModuleWriteFlags &= (~ModuleWriteDataSeg);
            }
        }
        return TRUE;

    case MemoryCallback:
        // We do not include any information here -> return FALSE
        return FALSE;

    case CancelCallback:
        break;
    }

    return FALSE;
}

bool MiniDumpHelper::write_dump(const char* path, PEXCEPTION_POINTERS exception_pointers, HANDLE process,
                               DWORD thread_id)
{
    if (!m_MiniDumpWriteDump)
        return false;

    HANDLE file = nullptr;
    file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                        nullptr);

    if (INVALID_HANDLE_VALUE == file) {
        std::string s = std::format("Error {}! CreateFile failed when writing a Minidump.", GetLastError());
        CRASHHANDLER_ERR(s.c_str());
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION exc_info;
    exc_info.ThreadId = thread_id;
    exc_info.ExceptionPointers = exception_pointers;
    exc_info.ClientPointers = TRUE;

    MINIDUMP_TYPE dump_type = MiniDumpNormal;
    MINIDUMP_CALLBACK_INFORMATION callback_info;
    MINIDUMP_CALLBACK_INFORMATION* callback_info_ptr = nullptr;

    // See http://www.debuginfo.com/articles/effminidumps2.html
    if (m_info_level == 0) {
        dump_type = MiniDumpWithIndirectlyReferencedMemory;
        callback_info_ptr = nullptr;
    }
    else if (m_info_level == 1) // medium information
    {
        dump_type = static_cast<MINIDUMP_TYPE>(
            MiniDumpWithPrivateReadWriteMemory | MiniDumpIgnoreInaccessibleMemory |
            MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithFullMemoryInfo |
            MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);

        callback_info.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)mini_dump_callback;
        callback_info.CallbackParam = this;
        callback_info_ptr = &callback_info;
    }
    else if (m_info_level == 2) // Maximal information
    {
        dump_type = static_cast<MINIDUMP_TYPE>(
            MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData |
            MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules |
            MiniDumpWithIndirectlyReferencedMemory);
        callback_info_ptr = nullptr;
    }

    SetLastError(0);
    auto process_id = GetProcessId(process);
    BOOL result = m_MiniDumpWriteDump(process, process_id, file, dump_type, &exc_info, nullptr, callback_info_ptr);
    if (!result) {
        std::string s = std::format("MiniDumpWriteDump {} {} {} failed with error {}", process_id, thread_id, file, GetLastError());
        CRASHHANDLER_ERR(s.c_str()); // ERROR_INVALID_PARAMETER?
    }

    CloseHandle(file);
    return result != FALSE;
}

MiniDumpHelper::MiniDumpHelper() :
    m_dbghelp_lib{L"Dbghelp.dll"}
{
    if (m_dbghelp_lib) {
        m_MiniDumpWriteDump = m_dbghelp_lib.get_proc_address<MiniDumpWriteDump_Type>("MiniDumpWriteDump");
    }
}

MiniDumpHelper::~MiniDumpHelper()
{
    m_MiniDumpWriteDump = nullptr;
}
