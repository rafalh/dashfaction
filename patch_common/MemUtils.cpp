#include <patch_common/MemUtils.h>
#include <windows.h>

void WriteMem(unsigned addr, const void* data, unsigned size)
{
    DWORD old_protect;

    VirtualProtect(reinterpret_cast<void*>(addr), size, PAGE_EXECUTE_READWRITE, &old_protect);
    std::memcpy(reinterpret_cast<void*>(addr), data, size);
    VirtualProtect(reinterpret_cast<void*>(addr), size, old_protect, NULL);
}

void UnprotectMem(void* ptr, unsigned len)
{
    DWORD old_protect;
    VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE, &old_protect);
}

void* AllocMemForCode(unsigned num_bytes)
{
    static auto heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
    return HeapAlloc(heap, 0, num_bytes);
}
