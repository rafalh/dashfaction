#pragma once

#include <stdint.h>
#include <cstring>

void WriteMem(unsigned Addr, const void *pValue, unsigned cbValue);

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

inline void WriteMemPtr(unsigned Addr, const void *Value, unsigned Repeat = 1)
{
    WriteMem(Addr, &Value, sizeof(Value), Repeat);
}

inline void WriteMemStr(unsigned Addr, const char *pStr)
{
    WriteMem(Addr, pStr, strlen(pStr) + 1);
}
