#include <MemUtils.h>
#include <Windows.h>

void WriteMem(unsigned Addr, const void *pValue, unsigned cbValue)
{
    DWORD dwOldProtect;

    VirtualProtect((void*)Addr, cbValue, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memcpy((void*)Addr, pValue, cbValue);
    VirtualProtect((void*)Addr, cbValue, dwOldProtect, NULL);
}
