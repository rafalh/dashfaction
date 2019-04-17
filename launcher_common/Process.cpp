#include "Process.h"
#include <cstddef>
#include <memory>
#include <psapi.h>
#include <stdexcept>
#include <string>
#include <windows.h>

#ifdef __GNUC__
template<typename T>
constexpr PBYTE PtrFromRva(T base, int rva)
{
    return reinterpret_cast<PBYTE>(base) + rva;
}
#else
#define PtrFromRva(base, rva) (((PBYTE)base) + rva)
#endif

Process::Process(int pid)
{
    m_handle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ |
                               SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                           FALSE, pid);
    if (!m_handle)
        throw std::runtime_error(std::string("OpenProcess failed: error ") + std::to_string(GetLastError()));
}

Process::~Process()
{
    CloseHandle(m_handle);
}

void* Process::ExecFunInternal(const char* module_name, const char* fun_name, void* arg, unsigned timeout)
{
    HMODULE module = FindModule(module_name);
    if (!module)
        throw std::runtime_error(std::string("Module not found: ") + module_name);

    void* proc = RemoteGetProcAddress(module, fun_name);
    if (!proc)
        throw std::runtime_error(std::string("Function not found: ") + fun_name);

    HANDLE thread = CreateRemoteThread(m_handle, NULL, 0, (LPTHREAD_START_ROUTINE)proc, arg, 0, NULL);
    if (!thread)
        throw std::runtime_error(std::string("CreateRemoteThread failed: error ") + std::to_string(GetLastError()));

    if (WaitForSingleObject(thread, timeout) != WAIT_OBJECT_0)
        throw std::runtime_error(std::string("Timed out when running remote function: ") + fun_name);

    DWORD exit_code;
    if (!GetExitCodeThread(thread, &exit_code))
        throw std::runtime_error(std::string("GetExitCodeThread failed: error ") + std::to_string(GetLastError()));

    CloseHandle(thread);
    return reinterpret_cast<void*>(exit_code);
}

void* Process::SaveData(const void* data, size_t size)
{
    PBYTE remote_ptr = (PBYTE)VirtualAllocEx(m_handle, 0, size, MEM_COMMIT, PAGE_READWRITE);
    if (!remote_ptr)
        throw std::runtime_error("VirtualAllocEx failed");

    if (!WriteProcessMemory(m_handle, remote_ptr, data, size, 0))
        throw std::runtime_error("WriteProcessMemory failed");

    return remote_ptr;
}

void Process::FreeMem(void* remote_ptr)
{
    VirtualFreeEx(m_handle, remote_ptr, 0, MEM_RELEASE);
}

HMODULE Process::FindModule(const char* module_name)
{
    DWORD num_bytes;
    if (!EnumProcessModules(m_handle, nullptr, 0, &num_bytes))
        throw std::runtime_error(std::string("EnumProcessModules failed: error1 ") + std::to_string(GetLastError()));

    int num_modules = num_bytes / sizeof(HMODULE);
    std::unique_ptr<HMODULE[]> modules(new HMODULE[num_modules]);
    if (!EnumProcessModules(m_handle, modules.get(), num_bytes, &num_bytes))
        throw std::runtime_error(std::string("EnumProcessModules failed: error2 ") + std::to_string(GetLastError()));

    char buf[MAX_PATH];
    for (int i = 0; i < num_modules; ++i) {
        if (GetModuleBaseName(m_handle, modules[i], buf, sizeof(buf)) && !_stricmp(module_name, buf))
            return modules[i];
    }

    SetLastError(ERROR_MOD_NOT_FOUND);
    return nullptr;
}

void Process::ReadMem(void* dest_ptr, const void* remote_src_ptr, size_t len)
{
    if (!ReadProcessMemory(m_handle, remote_src_ptr, dest_ptr, len, nullptr))
        throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));
}

void* Process::RemoteGetProcAddress(HMODULE module, const char* proc_name)
{
    if (!module)
        throw std::invalid_argument("module cannot be null");

    IMAGE_DOS_HEADER dos_hdr;
    if (!ReadProcessMemory(m_handle, module, &dos_hdr, sizeof(dos_hdr), nullptr))
        throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));

    if (dos_hdr.e_magic != IMAGE_DOS_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        throw std::runtime_error("Invalid DOS header signature");
    }

    IMAGE_NT_HEADERS nt_hdr;
    if (!ReadProcessMemory(m_handle, PtrFromRva(module, dos_hdr.e_lfanew), &nt_hdr, sizeof(nt_hdr), nullptr))
        throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));

    if (nt_hdr.Signature != IMAGE_NT_SIGNATURE) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        throw std::runtime_error("Invalid NT header signature");
    }

    IMAGE_DATA_DIRECTORY& export_dir_data = nt_hdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    DWORD export_dir_size = export_dir_data.Size;
    std::unique_ptr<std::byte[]> export_dir_buf(new std::byte[export_dir_size]);
    PIMAGE_EXPORT_DIRECTORY export_dir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(export_dir_buf.get());

    uintptr_t export_dir_addr = export_dir_data.VirtualAddress;
    if (!ReadProcessMemory(m_handle, PtrFromRva(module, export_dir_addr), export_dir, export_dir_size, NULL)) {
        throw std::runtime_error(std::string("ReadProcessMemory failed: error ") + std::to_string(GetLastError()));
    }

    PVOID base = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY*>(reinterpret_cast<PBYTE>(export_dir) - export_dir_addr);
    DWORD* names_array = reinterpret_cast<DWORD*>(PtrFromRva(base, export_dir->AddressOfNames));
    unsigned short* ordinals_array =
        reinterpret_cast<unsigned short*>(PtrFromRva(base, export_dir->AddressOfNameOrdinals));
    DWORD* addresses_array = reinterpret_cast<DWORD*>(PtrFromRva(base, export_dir->AddressOfFunctions));

    // Iterate over import descriptors/DLLs.
    for (unsigned i = 0; i < export_dir->NumberOfNames; ++i) {
        const char* current_name = reinterpret_cast<const char*>(PtrFromRva(base, names_array[i]));
        if (!strcmp(current_name, proc_name)) {
            DWORD rva = addresses_array[ordinals_array[i]];
            return PtrFromRva(module, rva);
        }
    }

    SetLastError(ERROR_MOD_NOT_FOUND);
    return nullptr;
}
