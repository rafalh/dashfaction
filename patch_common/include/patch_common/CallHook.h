#pragma once

#include <vector>
#include <patch_common/Traits.h>
#include <patch_common/Installable.h>

class CallHookImpl : public Installable
{
protected:
    std::vector<uintptr_t> m_call_op_addr_vec;
    void* m_target_fun_ptr;
    void* m_hook_fun_ptr;

    CallHookImpl(uintptr_t call_op_addr, void* hook_fun_ptr) :
        m_call_op_addr_vec{call_op_addr}, m_hook_fun_ptr{hook_fun_ptr}
    {}

    CallHookImpl(std::initializer_list<uintptr_t> call_op_addr, void* hook_fun_ptr) :
        m_call_op_addr_vec{call_op_addr}, m_hook_fun_ptr{hook_fun_ptr}
    {}

public:
    void install() override;
};

template<class T>
class CallHook;

template<class R, class... A>
class CallHook<R __cdecl(A...)> : public CallHookImpl
{
private:
    using FunType = R __cdecl (A...);

public:
    CallHook(uintptr_t call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    CallHook(std::initializer_list<uintptr_t> call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R call_target(A... a) const // NOLINT(modernize-use-nodiscard)
    {
        auto target_fun = reinterpret_cast<FunType*>(m_target_fun_ptr);
        return target_fun(a...);
    }
};

template<class R, class... A>
class CallHook<R __fastcall(A...)> : public CallHookImpl
{
private:
    using FunType = R __fastcall (A...);

public:
    CallHook(uintptr_t call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    CallHook(std::initializer_list<uintptr_t> call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R call_target(A... a) const // NOLINT(modernize-use-nodiscard)
    {
        auto target_fun = reinterpret_cast<FunType*>(m_target_fun_ptr);
        return target_fun(a...);
    }
};

template<class R, class... A>
class CallHook<R __stdcall(A...)> : public CallHookImpl
{
private:
    using FunType = R __stdcall (A...);

public:
    CallHook(uintptr_t call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    CallHook(std::initializer_list<uintptr_t> call_op_addr, FunType* hook_fun_ptr) :
        CallHookImpl(call_op_addr, reinterpret_cast<void*>(hook_fun_ptr))
    {}

    R call_target(A... a) const // NOLINT(modernize-use-nodiscard)
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
