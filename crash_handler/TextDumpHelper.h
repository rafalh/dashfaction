#include <windows.h>
#include <psapi.h>
#include <common/Win32Error.h>
#include <ostream>
#include <fstream>
#include <ctime>

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
public:
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
        //write_backtrace(out, ctx, process);
        write_stack_dump(out, ctx, process);
        write_modules(out, process);
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

    void write_backtrace(std::ostream& out, const CONTEXT& ctx, HANDLE process)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);

        out << "Backtrace:\n";
        out << StringFormat("%08X\n", ctx.Eip);

        uint32_t stack_buf[1024];
        uint8_t insn_buf[5];
        uintptr_t addr = ctx.Esp;
        while (addr < ctx.Esp + 0x1000) {
            SIZE_T bytes_read;
            bool success = ReadProcessMemory(process, reinterpret_cast<void*>(addr), stack_buf, sizeof(stack_buf), &bytes_read);
            if (!success || !bytes_read) {
                break;
            }

            for (unsigned i = 0; i < bytes_read / 4; ++i) {
                uint32_t val = stack_buf[i];
                if (val < reinterpret_cast<uintptr_t>(sys_info.lpMinimumApplicationAddress) ||
                    val > reinterpret_cast<uintptr_t>(sys_info.lpMaximumApplicationAddress)) {
                    continue;
                }

                uintptr_t potential_call_addr = stack_buf[i] - 5;
                success = ReadProcessMemory(process, reinterpret_cast<void*>(potential_call_addr), insn_buf, sizeof(insn_buf), &bytes_read);
                if (!success || !bytes_read) {
                    continue;
                }

                if (insn_buf[0] == 0xE8) {
                    out << StringFormat("%08X\n", potential_call_addr);
                }
            }

            addr += bytes_read;
        }
        out << std::endl;
    }

    void write_stack_dump(std::ostream& out, const CONTEXT& ctx, HANDLE process)
    {
        SIZE_T bytes_read;
        uintptr_t addr = ctx.Esp;
        uint32_t buf[1024];
        out << "Stack dump:\n";
        int cell_index = 0;

        while (addr < ctx.Esp + 0x1000) {
            if (!ReadProcessMemory(process, reinterpret_cast<void*>(addr), &buf, sizeof(buf), &bytes_read) || !bytes_read) {
                break;
            }

            for (unsigned i = 0; i < bytes_read / 4; ++i) {
                if (cell_index % 8 == 0) {
                    out << StringFormat("%08X: ", addr + i * 4);
                }

                out << StringFormat("%08X", buf[i]);
                bool new_line = ++cell_index % 8 == 0;
                out << (new_line ? '\n' : ' ');
            }

            addr += bytes_read;
        }
        out << std::endl;
    }

    void write_modules(std::ostream& out, HANDLE process)
    {
        out << "Modules:\n";

        HMODULE modules[256];
        DWORD bytes_needed;
        if (EnumProcessModules(process, modules, sizeof(modules), &bytes_needed)) {
            char mod_name[MAX_PATH];
            MODULEINFO mod_info;
            size_t num_modules = std::min<size_t>(bytes_needed / sizeof(HMODULE), std::size(modules));
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
