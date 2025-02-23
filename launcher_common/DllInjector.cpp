#include <xlog/xlog.h>
#include "DllInjector.h"

static FARPROC get_load_library_fun_ptr()
{
    // Note: kernel32.dll base address is the same for all processes during a single session
    // See: http://www.nynaeve.net/?p=198
    HMODULE kernel32_module = GetModuleHandleA("kernel32.dll");
    if (!kernel32_module)
        THROW_WIN32_ERROR();
    FARPROC load_library_ptr = GetProcAddress(kernel32_module, "LoadLibraryA");
    if (!load_library_ptr)
        THROW_WIN32_ERROR();
    return load_library_ptr;
}

static RemoteMemoryPtr str_dup_remote(Process& process, const char* str)
{
    unsigned str_size = strlen(str) + 1;
    RemoteMemoryPtr remote_buf = process.alloc_mem(str_size);
    try {
        // For some reason WriteProcessMemory returns 0, but memory is written
        process.write_mem(remote_buf, str, str_size);
    }
    catch (Win32Error& e) {
        std::fprintf(stderr, "%s\n", e.what());
    }

    return remote_buf;
}

static intptr_t get_exported_fun_offset_from_module_base(const char* dll_filename, const char* fun_name)
{
    HMODULE local_lib = LoadLibraryA(dll_filename);
    if (!local_lib)
        THROW_WIN32_ERROR();

    FARPROC proc_local_ptr = GetProcAddress(local_lib, fun_name);
    FreeLibrary(local_lib);
    if (!proc_local_ptr)
        THROW_WIN32_ERROR();
    return reinterpret_cast<intptr_t>(proc_local_ptr) - reinterpret_cast<intptr_t>(local_lib);
}

DWORD DllInjector::run_remote_fun(FARPROC fun_ptr, void* arg, int timeout)
{
    // Note: double cast is needed to fix cast-function-type GCC warning
    auto casted_fun_ptr = reinterpret_cast<LPTHREAD_START_ROUTINE>(reinterpret_cast<void(*)()>(fun_ptr));
    Thread thread = m_process.create_remote_thread(casted_fun_ptr, arg);
    thread.wait(timeout);
    return thread.get_exit_code();
}

DWORD DllInjector::load_library_remote(const char* dll_path, int timeout)
{
    RemoteMemoryPtr remote_dll_path = str_dup_remote(m_process, dll_path);
    FARPROC load_library_ptr = get_load_library_fun_ptr();

    xlog::info("Injecting DLL: {}\n", dll_path);
    DWORD remote_lib = run_remote_fun(load_library_ptr, remote_dll_path, timeout);
    if (!remote_lib)
        THROW_EXCEPTION("remote LoadLibrary failed");
    return remote_lib;
}

HMODULE DllInjector::inject_dll(const char* dll_filename, const char* init_fun_name, int timeout)
{
    try {
        // Find out Init function offset from DLL base address
        // Note: this calls LoadLibrary in current process and in case of problems with DLL loading it
        // will give a better error message than load_library_remote
        xlog::info("Finding Init function address");
        intptr_t init_fun_offset = get_exported_fun_offset_from_module_base(dll_filename, init_fun_name);
        // Load DLL in target process
        xlog::info("Loading library in remote process");
        DWORD remote_lib = load_library_remote(dll_filename, timeout);
        // Calculate address of Init function in remote process
        auto init_fun_remote_ptr = reinterpret_cast<FARPROC>(remote_lib + init_fun_offset);
        // Run Init function in remote process
        xlog::info("Calling Init function in remote process");
        DWORD exit_code = run_remote_fun(init_fun_remote_ptr, nullptr, timeout);
        if (!exit_code)
            THROW_EXCEPTION("Init function failed");
        return reinterpret_cast<HMODULE>(remote_lib);
    }
    catch (...) {
        std::throw_with_nested(std::runtime_error(std::string("failed to inject DLL: ") + dll_filename));
    }
}
