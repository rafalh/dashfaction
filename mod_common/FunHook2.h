#pragma once

#include <cstdint>
#include <log/Logger.h>
#include <subhook.h>

class FunHook2Impl {
protected:
    void *m_TargetFunPtr;
    void *m_HookFunPtr;
    subhook::Hook m_Subhook;

    FunHook2Impl(uintptr_t TargetFunAddr, void *HookFunPtr)
    {
        m_TargetFunPtr = reinterpret_cast<void*>(TargetFunAddr);
        m_HookFunPtr = HookFunPtr;
    }

public:
    void Install()
    {
        m_Subhook.Install(m_TargetFunPtr, m_HookFunPtr);
        if (!m_Subhook.GetTrampoline())
            ERR("trampoline is null for 0x%p", m_TargetFunPtr);
    }
};

template <class T>
class FunHook2;

template <class R, class... A>
class FunHook2<R __cdecl(A...)> : public FunHook2Impl {
private:
    typedef R __cdecl FunType(A...);

public:
    FunHook2(uintptr_t TargetFunAddr, FunType *HookFunPtr)
        : FunHook2Impl(TargetFunAddr, reinterpret_cast<void*>(HookFunPtr)) {}

    R CallTarget(A... a)
    {
        auto TrampolinePtr = reinterpret_cast<FunType*>(m_Subhook.GetTrampoline());
        return TrampolinePtr(a...);
    }
};

template <class R, class... A>
class FunHook2<R __fastcall(A...)> : public FunHook2Impl {
private:
    typedef R __fastcall FunType(A...);

public:
    FunHook2(uintptr_t TargetFunAddr, FunType *HookFunPtr)
        : FunHook2Impl(TargetFunAddr, reinterpret_cast<void*>(HookFunPtr)) {}

    R CallTarget(A... a)
    {
        auto TrampolinePtr = reinterpret_cast<FunType*>(m_Subhook.GetTrampoline());
        return TrampolinePtr(a...);
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template <class T>
FunHook2(uintptr_t Addr, T)->FunHook2<typename std::remove_pointer_t<decltype(+std::declval<T>())>>;
#endif
