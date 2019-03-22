#pragma once

#include <cstdint>
#include <MemUtils.h>
#include <AsmOpcodes.h>
#include <log/Logger.h>

class CallHook2Impl {
protected:
    uintptr_t m_call_op_addr;
    void *m_target_fun_ptr;
    void *m_hook_fun_ptr;

    CallHook2Impl(uintptr_t call_op_addr, void *hook_fun_ptr) :
        m_call_op_addr{call_op_addr}, m_hook_fun_ptr{hook_fun_ptr} {}

public:
    void Install()
    {
        uint8_t Opcode = *reinterpret_cast<uint8_t*>(m_call_op_addr);
        if (Opcode != ASM_LONG_CALL_REL) {
            ERR("not a call at 0x%p", m_call_op_addr);
            return;
        }

        intptr_t call_offset = *reinterpret_cast<intptr_t*>(m_call_op_addr + 1);
        int call_op_size = 1 + sizeof(uintptr_t);
        m_target_fun_ptr = reinterpret_cast<void*>(m_call_op_addr + call_op_size + call_offset);
        intptr_t new_offset = reinterpret_cast<intptr_t>(m_hook_fun_ptr) - m_call_op_addr - call_op_size;
        WriteMem<i32>(m_call_op_addr + 1, new_offset);
    }
};

template <class T>
class CallHook2;

template <class R, class... A>
class CallHook2<R __cdecl(A...)> : public CallHook2Impl {
private:
    typedef R __cdecl FunType(A...);

public:
    CallHook2(uintptr_t call_op_addr, FunType *hook_fun_ptr)
        : CallHook2Impl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr)) {}

    R CallTarget(A... a) const
    {
        auto TargetFun = reinterpret_cast<FunType*>(m_target_fun_ptr);
        return TargetFun(a...);
    }
};

template <class R, class... A>
class CallHook2<R __fastcall(A...)> : public CallHook2Impl {
private:
    typedef R __fastcall FunType(A...);

public:
    CallHook2(uintptr_t call_op_addr, FunType *hook_fun_ptr)
        : CallHook2Impl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr)) {}

    R CallTarget(A... a) const
    {
        auto TargetFun = reinterpret_cast<FunType*>(m_target_fun_ptr);
        return TargetFun(a...);
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template<class T> CallHook2(uintptr_t addr, T) -> CallHook2<typename std::remove_pointer_t<decltype(+std::declval<T>())>>;
#endif
