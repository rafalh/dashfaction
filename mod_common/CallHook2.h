#pragma once

#include <cstdint>
#include <MemUtils.h>
#include <AsmOpcodes.h>
#include <log/Logger.h>

class CallHook2Impl {
protected:
    uintptr_t m_CallOpAddr;
    void *m_TargetFunPtr;
    void *m_HookFunPtr;

    CallHook2Impl(uintptr_t CallOpAddr, void *HookFunPtr) :
        m_CallOpAddr{CallOpAddr}, m_HookFunPtr{HookFunPtr} {}

public:
    void Install()
    {
        uint8_t Opcode = *reinterpret_cast<uint8_t*>(m_CallOpAddr);
        if (Opcode != ASM_LONG_CALL_REL) {
            ERR("not a call at 0x%p", m_CallOpAddr);
            return;
        }

        intptr_t CallOffset = *reinterpret_cast<intptr_t*>(m_CallOpAddr + 1);
        int CallInstructionSize = 1 + sizeof(uintptr_t);
        m_TargetFunPtr = reinterpret_cast<void*>(m_CallOpAddr + CallInstructionSize + CallOffset);
        intptr_t NewOffset = reinterpret_cast<intptr_t>(m_HookFunPtr) - m_CallOpAddr - CallInstructionSize;
        WriteMemInt32(m_CallOpAddr + 1, NewOffset);
    }
};

template <class T>
class CallHook2;

template <class R, class... A>
class CallHook2<R __cdecl(A...)> : public CallHook2Impl {
private:
    typedef R __cdecl FunType(A...);

public:
    CallHook2(uintptr_t CallOpAddr, FunType *HookFunPtr)
        : CallHook2Impl(CallOpAddr, reinterpret_cast<void*>(HookFunPtr)) {}

    R CallTarget(A... a) const
    {
        auto TargetFun = reinterpret_cast<FunType*>(m_TargetFunPtr);
        return TargetFun(a...);
    }
};

template <class R, class... A>
class CallHook2<R __fastcall(A...)> : public CallHook2Impl {
private:
    typedef R __fastcall FunType(A...);

public:
    CallHook2(uintptr_t CallOpAddr, FunType *HookFunPtr)
        : CallHook2Impl(CallOpAddr, reinterpret_cast<void*>(HookFunPtr)) {}

    R CallTarget(A... a) const
    {
        auto TargetFun = reinterpret_cast<FunType*>(m_TargetFunPtr);
        return TargetFun(a...);
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template<class T> CallHook2(uintptr_t Addr, T) -> CallHook2<typename std::remove_pointer_t<decltype(+std::declval<T>())>>;
#endif
