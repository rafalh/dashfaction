#pragma once

#include <patch_common/ShortTypes.h>
#include <patch_common/Traits.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/MemUtils.h>
#include <cstdint>
#include <xlog/xlog.h>

class CallHookImpl
{
protected:
    std::vector<uintptr_t> m_call_op_addr_vec;
    void* m_target_fun_ptr;
    void* m_hook_fun_ptr;

    CallHookImpl(uintptr_t call_op_addr, void* hook_fun_ptr) :
        m_call_op_addr_vec{{call_op_addr}}, m_hook_fun_ptr{hook_fun_ptr}
    {}

    CallHookImpl(std::initializer_list<uintptr_t> call_op_addr, void* hook_fun_ptr) :
        m_call_op_addr_vec{call_op_addr}, m_hook_fun_ptr{hook_fun_ptr}
    {}

public:
    void Install()
    {
        m_target_fun_ptr = nullptr;
        for (auto addr : m_call_op_addr_vec) {
            uint8_t opcode = *reinterpret_cast<uint8_t*>(addr);
            if (opcode != asm_opcodes::call_rel_long) {
                xlog::error("not a call at 0x%X", addr);
                return;
            }

            intptr_t call_offset = *reinterpret_cast<intptr_t*>(addr + 1);
            int call_op_size = 1 + sizeof(uintptr_t);
            auto call_addr = reinterpret_cast<void*>(addr + call_op_size + call_offset);
            if (!m_target_fun_ptr) {
                m_target_fun_ptr = call_addr;
            }
            else if (call_addr != m_target_fun_ptr) {
                xlog::error("call target function differs at 0x%X", addr);
            }

            intptr_t new_offset = reinterpret_cast<intptr_t>(m_hook_fun_ptr) - addr - call_op_size;
            WriteMem<i32>(addr + 1, new_offset);
        }
    }
};

template<class T>
class CallHook;

template<class R, class... A>
class CallHook<R __cdecl(A...)> : public CallHookImpl
{
private:
    typedef R __cdecl FunType(A...);

public:
    CallHook(uintptr_t call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    CallHook(std::initializer_list<uintptr_t> call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R CallTarget(A... a) const
    {
        auto target_fun = reinterpret_cast<FunType*>(m_target_fun_ptr);
        return target_fun(a...);
    }
};

template<class R, class... A>
class CallHook<R __fastcall(A...)> : public CallHookImpl
{
private:
    typedef R __fastcall FunType(A...);

public:
    CallHook(uintptr_t call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    CallHook(std::initializer_list<uintptr_t> call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R CallTarget(A... a) const
    {
        auto target_fun = reinterpret_cast<FunType*>(m_target_fun_ptr);
        return target_fun(a...);
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template<class T>
CallHook(uintptr_t addr, T)->CallHook<typename function_traits<T>::f_type>;
#endif
