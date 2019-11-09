#include <windows.h>
#include <psapi.h>
#include <common/Win32Error.h>
#include <ostream>
#include <fstream>
#include <ctime>
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <optional>
#include <cstddef>

inline std::string StringFormat(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int size = vsnprintf(nullptr, 0, format, args) + 1;// Extra space for '\0'
    va_end(args);
    std::unique_ptr<char[]> buf(new char[size]);
    va_start(args, format);
    vsnprintf(buf.get(), size, format, args);
    va_end(args);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

class TextDumpHelper {
private:
    SYSTEM_INFO m_sys_info;
    std::vector<MEMORY_BASIC_INFORMATION> m_memory_map;
    std::unordered_map<size_t, std::unique_ptr<uint32_t[]>> m_page_cache;

public:
    TextDumpHelper() {
        GetSystemInfo(&m_sys_info);
    }

    void write_dump(const char* filename, EXCEPTION_POINTERS* exception_pointers, HANDLE process)
    {
        std::fstream out(filename, std::fstream::out);
        write_dump(out, exception_pointers, process);
    }

    void write_dump(std::ostream& out, EXCEPTION_POINTERS* exception_pointers, HANDLE process)
    {
        auto [exc_rec, ctx] = fetch_exception_info(exception_pointers, process);

        out << "Unhandled exception\n";
        write_current_time(out);
        if (exc_rec.ExceptionCode == STATUS_ACCESS_VIOLATION && exc_rec.NumberParameters >= 2) {
            auto prefix = exc_rec.ExceptionInformation[0] ? "Write to" : "Read from";
            out << prefix << StringFormat(" location %08X caused an access violation.\n", exc_rec.ExceptionInformation[1]);
        }
        out << std::endl;
        write_exception_info(out, exc_rec);
        write_context(out, ctx);

        m_memory_map = fetch_memory_map(process);

        write_backtrace(out, ctx, process);
        write_stack_dump(out, ctx, process);
        write_modules(out, process);
        write_memory_map(out);
    }

private:
    std::tuple<EXCEPTION_RECORD, CONTEXT> fetch_exception_info(EXCEPTION_POINTERS* exception_pointers, HANDLE process)
    {
        EXCEPTION_POINTERS exception_pointers_local;
        SIZE_T bytes_read;

        if (!ReadProcessMemory(process, exception_pointers, &exception_pointers_local, sizeof(exception_pointers_local), &bytes_read)) {
            THROW_WIN32_ERROR();
        }

        CONTEXT context;
        if (!ReadProcessMemory(process, exception_pointers_local.ContextRecord, &context, sizeof(context), &bytes_read)) {
            THROW_WIN32_ERROR();
        }

        EXCEPTION_RECORD exception_record;
        if (!ReadProcessMemory(process, exception_pointers_local.ExceptionRecord, &exception_record, sizeof(exception_record), &bytes_read)) {
            THROW_WIN32_ERROR();
        }

        return {exception_record, context};
    }

    void write_current_time(std::ostream& out)
    {
        time_t rawtime;
        struct tm* timeinfo;

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        out << "Date and time: " << asctime(timeinfo);
    }

    void write_exception_info(std::ostream& out, const EXCEPTION_RECORD& exc_rec)
    {
        out << "Exception Record:\n";
        out << "  ExceptionCode = " << StringFormat("%08X", exc_rec.ExceptionCode) << "\n";
        out << "  ExceptionFlags = " << StringFormat("%08X", exc_rec.ExceptionFlags) << "\n";
        out << "  ExceptionAddress = " << StringFormat("%08X", exc_rec.ExceptionAddress) << "\n";
        for (unsigned i = 0; i < exc_rec.NumberParameters; ++i)
            out << "  ExceptionInformation[" << i << "] = " << StringFormat("%08X", exc_rec.ExceptionInformation[i]) << "\n";
        out << std::endl;
    }

    void write_context(std::ostream& out, const CONTEXT& ctx)
    {
        out << "Context:\n";
        out << StringFormat("EIP=%08X EFLAGS=%08X\n", ctx.Eip, ctx.EFlags);
        out << StringFormat("EAX=%08X EBX=%08X ECX=%08X EDX=%08X\n", ctx.Eax, ctx.Ebx, ctx.Ecx, ctx.Edx);
        out << StringFormat("ESP=%08X EBP=%08X ESI=%08X EDI=%08X\n", ctx.Esp, ctx.Ebp, ctx.Esi, ctx.Edi);
        out << std::endl;
    }

    std::vector<MEMORY_BASIC_INFORMATION> fetch_memory_map(HANDLE process)
    {
        MEMORY_BASIC_INFORMATION mbi;
        std::vector<MEMORY_BASIC_INFORMATION> memory_map;
        const char* addr = reinterpret_cast<const char*>(m_sys_info.lpMinimumApplicationAddress);
        while (VirtualQueryEx(process, addr, &mbi, sizeof(mbi)) && addr <= m_sys_info.lpMaximumApplicationAddress) {
            memory_map.push_back(mbi);
            addr = reinterpret_cast<const char*>(mbi.BaseAddress) + mbi.RegionSize;
        }
        return memory_map;
    }

    void write_memory_map(std::ostream& out)
    {
        out << "Memory map:\n";
        for (auto& mbi: m_memory_map) {
            out << StringFormat("%08X: State %08X Protect %08X Type %08X RegionSize %08X\n",
                mbi.BaseAddress, mbi.State, mbi.Protect, mbi.Type, mbi.RegionSize);
            //out << StringFormat("  AllocBase %x AllocProtect %x\n", mbi.AllocationBase, mbi.AllocationProtect);
        }
        out << std::endl;
    }

    bool is_executable_addr(void* addr)
    {
        if (addr < m_sys_info.lpMinimumApplicationAddress || addr > m_sys_info.lpMaximumApplicationAddress) {
            return false;
        }
        auto mem_map_it = find_in_memory_map(addr);
        if (mem_map_it == m_memory_map.end()) {
            return false;
        }
        auto protect = mem_map_it->Protect;
        return protect == PAGE_EXECUTE
            || protect == PAGE_EXECUTE_READ
            || protect == PAGE_EXECUTE_READWRITE
            || protect == PAGE_EXECUTE_WRITECOPY;
    }

    void* load_page_into_cache(uintptr_t page_addr, HANDLE process)
    {
        auto it = m_page_cache.find(page_addr);
        if (it == m_page_cache.end()) {
            SIZE_T bytes_read;
            auto page_buf = std::make_unique<uint32_t[]>(m_sys_info.dwPageSize / sizeof(uint32_t));
            bool read_success = ReadProcessMemory(process, reinterpret_cast<void*>(page_addr), page_buf.get(),
                m_sys_info.dwPageSize, &bytes_read);
            if (!read_success || !bytes_read) {
                return nullptr;
            }
            it = m_page_cache.insert(std::make_pair(page_addr, std::move(page_buf))).first;
        }
        return it->second.get();
    }

    bool read_mem(void* buf, void* addr, size_t size, HANDLE process)
    {
        auto out_buf_bytes = reinterpret_cast<std::byte*>(buf);
        auto addr_uint = reinterpret_cast<uintptr_t>(addr);

        while (size > 0) {
            uintptr_t offset = addr_uint % m_sys_info.dwPageSize;
            uintptr_t page_addr = addr_uint - offset;
            std::byte* page_data = reinterpret_cast<std::byte*>(load_page_into_cache(page_addr, process));
            if (page_data == nullptr) {
                return false;
            }
            size_t bytes_to_copy = std::min<size_t>(size, m_sys_info.dwPageSize - offset);
            std::copy(page_data + offset, page_data + offset + bytes_to_copy, out_buf_bytes);
            size -= bytes_to_copy;
            addr_uint += bytes_to_copy;
        }
        return true;
    }

    void write_backtrace(std::ostream& out, const CONTEXT& ctx, HANDLE process)
    {
        out << "Backtrace:\n";
        out << StringFormat("%08X\n", ctx.Eip);

        SIZE_T bytes_read;
        uintptr_t addr = ctx.Esp & ~3;
        uintptr_t stack_max_addr = reinterpret_cast<uintptr_t>(find_stack_max_addr(ctx));
        size_t stack_size = stack_max_addr - addr;
        auto stack_buf = std::make_unique<uint32_t[]>(stack_size / sizeof(uint32_t));

        bool read_success = ReadProcessMemory(process, reinterpret_cast<void*>(addr), stack_buf.get(), stack_size, &bytes_read);
        if (!read_success || !bytes_read) {
            return;
        }


        uint8_t insn_buf[5];
        size_t dwords_read = bytes_read / sizeof(uint32_t);
        for (unsigned i = 0; i < dwords_read; ++i) {
            uint32_t potential_call_addr = stack_buf[i] - 5;
            if (!is_executable_addr(reinterpret_cast<void*>(potential_call_addr))) {
                continue;
            }

            if (!read_mem(insn_buf, reinterpret_cast<std::byte*>(potential_call_addr), sizeof(insn_buf), process)) {
                continue;
            }

            if (insn_buf[0] == 0xE8) {
                out << StringFormat("%08X\n", static_cast<unsigned>(potential_call_addr));
            }
        }

        out << std::endl;
    }

    std::vector<MEMORY_BASIC_INFORMATION>::const_iterator find_in_memory_map(void* addr) const
    {
        auto first = std::lower_bound(m_memory_map.begin(), m_memory_map.end(), addr, [](const auto& e, const auto& v) {
            return e.BaseAddress < v;
        });
        if (first == m_memory_map.end()) {
            return m_memory_map.end();
        }
        if (first->BaseAddress > addr) {
            if (first == m_memory_map.begin()) {
                return m_memory_map.end();
            }
            --first;
        }
        assert(first->BaseAddress <= addr);
        return first;
    }

    const MEMORY_BASIC_INFORMATION* find_page_in_memory_map(void* addr)
    {
        auto first = std::lower_bound(m_memory_map.begin(), m_memory_map.end(), addr, [](const auto& e, const auto& v) {
            return e.BaseAddress < v;
        });
        if (first == m_memory_map.end()) {
            return nullptr;
        }
        if (first->BaseAddress > addr) {
            if (first == m_memory_map.begin()) {
                return nullptr;
            }
            --first;
        }
        assert(first->BaseAddress <= addr);
        return &*first;
    }

    void* find_stack_max_addr(const CONTEXT& ctx)
    {
        void* addr = reinterpret_cast<void*>(ctx.Esp);
        auto it = find_in_memory_map(addr);

        while (it != m_memory_map.end()) {
            if (it->State != MEM_COMMIT || it->Protect != PAGE_READWRITE) {
                return it->BaseAddress;
            }
            ++it;
        }
        return addr;
    }

    void write_stack_dump(std::ostream& out, const CONTEXT& ctx, HANDLE process)
    {
        SIZE_T bytes_read;
        uintptr_t addr = ctx.Esp & ~3;
        uintptr_t stack_max_addr = reinterpret_cast<uintptr_t>(find_stack_max_addr(ctx));
        size_t stack_size = stack_max_addr - addr;
        auto buf = std::make_unique<uint32_t[]>(stack_size / sizeof(uint32_t));
        out << "Stack dump:\n";

        bool read_success = ReadProcessMemory(process, reinterpret_cast<void*>(addr), buf.get(), stack_size, &bytes_read);
        if (!read_success || !bytes_read) {
            return;
        }

        int col = 0;
        size_t dwords_read = bytes_read / sizeof(uint32_t);
        for (unsigned i = 0; i < dwords_read; ++i) {
            if (col == 0) {
                out << StringFormat("%08X: ", addr + i * 4);
            }

            out << StringFormat("%08X", buf[i]);
            col = (col + 1) % 8;
            out << (col == 0 ? '\n' : ' ');
        }
        out << std::endl;
    }

    void write_modules(std::ostream& out, HANDLE process)
    {
        out << "Modules:\n";

        HMODULE modules[256];
        DWORD bytes_needed;
        if (EnumProcessModules(process, modules, sizeof(modules), &bytes_needed)) {
            size_t num_modules = std::min<size_t>(bytes_needed / sizeof(HMODULE), std::size(modules));
            std::sort(modules, modules + num_modules);

            char mod_name[MAX_PATH];
            MODULEINFO mod_info;
            for (size_t i = 0; i < num_modules; i++) {
                if (GetModuleFileNameEx(process, modules[i], mod_name, std::size(mod_name)) &&
                    GetModuleInformation(process, modules[i], &mod_info, sizeof(mod_info))) {
                    uintptr_t start_addr = reinterpret_cast<uintptr_t>(mod_info.lpBaseOfDll);
                    uintptr_t end_addr = start_addr + mod_info.SizeOfImage;
                    out << StringFormat("%08X - %08X: %s\n", start_addr, end_addr, mod_name);
                }
            }
        }

        out << std::endl;
    }
};
