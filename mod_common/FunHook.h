#pragma once

#include "Traits.h"
#include <cstdint>
#include <log/Logger.h>
#include <subhook.h>

class FunHookImpl
{
protected:
    void* m_target_fun_ptr;
    void* m_hook_fun_ptr;
    subhook::Hook m_subhook;

    FunHookImpl(uintptr_t target_fun_addr, void* hook_fun_ptr)
    {
        m_target_fun_ptr = reinterpret_cast<void*>(target_fun_addr);
        m_hook_fun_ptr = hook_fun_ptr;
    }

public:
    void Install()
    {
        m_subhook.Install(m_target_fun_ptr, m_hook_fun_ptr);
        if (!m_subhook.GetTrampoline())
            ERR("trampoline is null for 0x%p", m_target_fun_ptr);
    }
};

template<class T>
class FunHook;

template<class R, class... A>
class FunHook<R __cdecl(A...)> : public FunHookImpl
{
private:
    typedef R __cdecl FunType(A...);

public:
    FunHook(uintptr_t target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(target_fun_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R CallTarget(A... a)
    {
        auto trampoline_ptr = reinterpret_cast<FunType*>(m_subhook.GetTrampoline());
        return trampoline_ptr(a...);
    }
};

template<class R, class... A>
class FunHook<R __fastcall(A...)> : public FunHookImpl
{
private:
    typedef R __fastcall FunType(A...);

public:
    FunHook(uintptr_t target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(target_fun_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R CallTarget(A... a)
    {
        auto trampoline_ptr = reinterpret_cast<FunType*>(m_subhook.GetTrampoline());
        return trampoline_ptr(a...);
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template<class T>
FunHook(uintptr_t addr, T)->FunHook<typename function_traits<T>::f_type>;
#endif
