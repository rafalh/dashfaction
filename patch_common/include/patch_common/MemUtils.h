#pragma once

#include <cstdint>

template<typename T>
struct TypeIdentity
{
    using type = T;
};

void write_mem(unsigned addr, const void* data, unsigned size);
void unprotect_mem(void* ptr, unsigned len);
size_t get_instruction_len(void* ptr);

template<typename T>
void write_mem(uintptr_t addr, typename TypeIdentity<T>::type value)
{
    write_mem(addr, &value, sizeof(value));
}

inline void write_mem(unsigned addr, const void* data, unsigned size, unsigned num_repeat)
{
    while (num_repeat > 0) {
        write_mem(addr, data, size);
        addr += size;
        --num_repeat;
    }
}

template<typename T>
inline void write_mem_ptr(unsigned addr, T* value)
{
    write_mem(addr, &value, sizeof(T*));
}

template<typename T>
constexpr T& addr_as_ref(uintptr_t addr)
{
    return *reinterpret_cast<T*>(addr);
}

template<typename T>
T& struct_field_ref(void* struct_ptr, size_t offset)
{
    auto addr = reinterpret_cast<uintptr_t>(struct_ptr) + offset;
    return *reinterpret_cast<T*>(addr);
}

// Note: references will not be properly passed as argments unless type is specified explicity.
// The reason for that is that argument deduction strips reference from type.
class AddrCaller
{
    uintptr_t addr_;

public:
    constexpr AddrCaller(uintptr_t addr) : addr_(addr)
    {}

    template<typename RetVal = void, typename... A>
    constexpr RetVal c_call(A... args)
    {
        return addr_as_ref<RetVal __cdecl(A...)>(addr_)(args...);
    }

    template<typename RetVal = void, typename... A>
    constexpr RetVal this_call(A... args)
    {
        return reinterpret_cast<RetVal(__thiscall*)(A...)>(addr_)(args...);
    }

    template<typename RetVal = void, typename... A>
    constexpr RetVal fast_call(A... args)
    {
        return addr_as_ref<RetVal __fastcall(A...)>(addr_)(args...);
    }

    template<typename RetVal = void, typename... A>
    constexpr RetVal std_call(A... args)
    {
        return addr_as_ref<RetVal __stdcall(A...)>(addr_)(args...);
    }
};
