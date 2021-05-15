#include <patch_common/MemUtils.h>
#include <windows.h>
#include <xlog/xlog.h>
#include <cstring>

void write_mem(unsigned addr, const void* data, unsigned size)
{
    DWORD old_protect;

    if (!VirtualProtect(reinterpret_cast<void*>(addr), size, PAGE_EXECUTE_READWRITE, &old_protect)) {
        xlog::warn("VirtualProtect failed: addr %x size %x error %lu", addr, size, GetLastError());
    }
    std::memcpy(reinterpret_cast<void*>(addr), data, size);
    VirtualProtect(reinterpret_cast<void*>(addr), size, old_protect, nullptr);
}

void unprotect_mem(void* ptr, unsigned len)
{
    DWORD old_protect;
    if (!VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE, &old_protect)) {
        xlog::warn("VirtualProtect failed: addr %p size %x error %lu", ptr, len, GetLastError());
    }
}

extern "C" size_t subhook_disasm(void *src, int32_t *reloc_op_offset);

size_t get_instruction_len(void* ptr)
{
    return subhook_disasm(ptr, nullptr);
}
