#pragma once

#include <cstring>
#include <stdint.h>

template<typename T>
struct TypeIdentity
{
    using type = T;
};

void WriteMem(unsigned addr, const void* data, unsigned size);
void UnprotectMem(void* ptr, unsigned len);
void* AllocMemForCode(unsigned num_bytes);

template<typename T>
void WriteMem(uintptr_t addr, typename TypeIdentity<T>::type value)
{
    WriteMem(addr, &value, sizeof(value));
}

inline void WriteMem(unsigned addr, const void* data, unsigned size, unsigned num_repeat)
{
    while (num_repeat > 0) {
        WriteMem(addr, data, size);
        addr += size;
        --num_repeat;
    }
}

template<typename T>
inline void WriteMemPtr(unsigned addr, T* value)
{
    WriteMem(addr, &value, sizeof(value));
}

inline void WriteMemStr(unsigned addr, const char* str)
{
    WriteMem(addr, str, std::strlen(str) + 1);
}

template<typename T>
constexpr T& AddrAsRef(uintptr_t addr)
{
    return *reinterpret_cast<T*>(addr);
}

template<typename T>
T& StructFieldRef(void* struct_ptr, size_t offset)
{
    auto addr = reinterpret_cast<uintptr_t>(struct_ptr) + offset;
    return *reinterpret_cast<T*>(addr);
}

template<typename... A>
void *CallAddr(uintptr_t addr, A... args)
{
    return AddrAsRef<void*(A...)>(addr)(args...);
}
