#include <patch_common/MemUtils.h>
#include <windows.h>

void WriteMem(unsigned Addr, const void* pValue, unsigned cbValue)
{
    DWORD dwOldProtect;

    VirtualProtect((void*)Addr, cbValue, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memcpy((void*)Addr, pValue, cbValue);
    VirtualProtect((void*)Addr, cbValue, dwOldProtect, NULL);
}

void UnprotectMem(void* Ptr, unsigned Len)
{
    DWORD dwOldProtect;
    VirtualProtect(Ptr, Len, PAGE_EXECUTE_READWRITE, &dwOldProtect);
}

void* AllocMemForCode(unsigned num_bytes)
{
    static auto heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
    return HeapAlloc(heap, 0, num_bytes);
}
