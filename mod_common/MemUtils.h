#pragma once

#include <stdint.h>
#include <cstring>

template<typename T>
struct TypeIdentity {
    using type = T;
};

void WriteMem(unsigned Addr, const void *pValue, unsigned cbValue);
void UnprotectMem(void *Ptr, unsigned Len);
void *AllocMemForCode(unsigned num_bytes);

template<typename T>
void WriteMem(uintptr_t addr, typename TypeIdentity<T>::type value)
{
    WriteMem(addr, &value, sizeof(value));
}

inline void WriteMem(unsigned Addr, const void *pValue, unsigned cbValue, unsigned cRepeat)
{
    while (cRepeat > 0)
    {
        WriteMem(Addr, pValue, cbValue);
        Addr += cbValue;
        --cRepeat;
    }
}

template<typename T>
inline void WriteMemPtr(unsigned Addr, T *Value)
{
    WriteMem(Addr, &Value, sizeof(Value));
}

inline void WriteMemStr(unsigned Addr, const char *pStr)
{
    WriteMem(Addr, pStr, strlen(pStr) + 1);
}

template<typename T>
constexpr T &AddrAsRef(uintptr_t Addr)
{
    return *(T*)Addr;
}

template<typename T>
T &StructFieldRef(void *StructPtr, size_t Offset)
{
    auto Addr = reinterpret_cast<uintptr_t>(StructPtr) + Offset;
    return *reinterpret_cast<T*>(Addr);
}
