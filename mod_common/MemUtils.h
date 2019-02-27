#pragma once

#include <stdint.h>
#include <cstring>

void WriteMem(unsigned Addr, const void *pValue, unsigned cbValue);
void UnprotectMem(void *Ptr, unsigned Len);

inline void WriteMem(unsigned Addr, const void *pValue, unsigned cbValue, unsigned cRepeat)
{
    while (cRepeat > 0)
    {
        WriteMem(Addr, pValue, cbValue);
        Addr += cbValue;
        --cRepeat;
    }
}

inline void WriteMemUInt32(unsigned Addr, uint32_t Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemInt32(unsigned Addr, int32_t Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemUInt16(unsigned Addr, uint16_t Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemInt16(unsigned Addr, int16_t Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemUInt8(unsigned Addr, uint8_t Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemInt8(unsigned Addr, int8_t Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemFloat(unsigned Addr, float Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

template<typename T>
inline void WriteMemPtr(unsigned Addr, T *Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
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
