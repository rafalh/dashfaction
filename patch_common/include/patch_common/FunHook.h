#pragma once

#include <cstdint>
#include <subhook.h>
#include <patch_common/Traits.h>
#include <patch_common/Installable.h>

class FunHookImpl : public Installable
{
protected:
    void* m_trampoline_ptr;
    void* m_target_fun_ptr;
    void* m_hook_fun_ptr;
    subhook::Hook m_subhook;

    FunHookImpl(uintptr_t target_fun_addr, void* hook_fun_ptr)
    {
        m_target_fun_ptr = reinterpret_cast<void*>(target_fun_addr);
        m_hook_fun_ptr = hook_fun_ptr;
    }

public:
    void install() override;

    void set_addr(uintptr_t target_fun_addr)
    {
        m_target_fun_ptr = reinterpret_cast<void*>(target_fun_addr);
    }
};

template<class T>
class FunHook;

template<class R, class... A>
class FunHook<R __cdecl(A...)> : public FunHookImpl
{
private:
    using FunType = R __cdecl(A...);

public:
    FunHook(uintptr_t target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(target_fun_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    FunHook(FunType* target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(reinterpret_cast<uintptr_t>(target_fun_addr), reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R call_target(A... a) const // NOLINT(modernize-use-nodiscard)
    {
        auto trampoline_ptr = reinterpret_cast<FunType*>(m_trampoline_ptr);
        return trampoline_ptr(a...);
    }
};

template<class R, class... A>
class FunHook<R __fastcall(A...)> : public FunHookImpl
{
private:
    using FunType = R __fastcall(A...);

public:
    FunHook(uintptr_t target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(target_fun_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    FunHook(FunType* target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(reinterpret_cast<uintptr_t>(target_fun_addr), reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R call_target(A... a) const
    {
        auto trampoline_ptr = reinterpret_cast<FunType*>(m_trampoline_ptr);
        return trampoline_ptr(a...);
    }
};

template<class R, class... A>
class FunHook<R __stdcall(A...)> : public FunHookImpl
{
private:
    using FunType = R __stdcall(A...);

public:
    FunHook(uintptr_t target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(target_fun_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    FunHook(FunType* target_fun_addr, FunType* hook_fun_ptr) :
        FunHookImpl(reinterpret_cast<uintptr_t>(target_fun_addr), reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R call_target(A... a) const
    {
        auto trampoline_ptr = reinterpret_cast<FunType*>(m_trampoline_ptr);
        return trampoline_ptr(a...);
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template<class T>
FunHook(uintptr_t addr, T)->FunHook<typename function_traits<T>::f_type>;
#endif
