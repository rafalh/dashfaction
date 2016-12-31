#pragma once

#include "FunPtr.h"
#include "hooks/HookFunction.h"

template<uintptr_t DefaultAddr, typename RetType, typename... ArgTypes>
class HookableFunPtr : public FunPtr<DefaultAddr, RetType, ArgTypes...>
{
public:
    void hook(RetType(*pfn)(ArgTypes...))
    {
        uintptr_t addr = FunPtr<DefaultAddr, RetType, ArgTypes...>::m_addr;
        auto *hook = new HookFunction<RetType(*)(ArgTypes...)>(addr, pfn);
        m_hook.reset(hook);
    }

    RetType callOrig(ArgTypes... args)
    {
        return m_hook->GetTrampoline()(args...);
    }

protected:
    std::unique_ptr<HookFunction<RetType(*)(ArgTypes...)>> m_hook;
};

template<uintptr_t DefaultAddr, typename... ArgTypes>
class HookableFunPtr<DefaultAddr, void, ArgTypes...> : public FunPtr<DefaultAddr, void, ArgTypes...>
{
public:
    void hook(void(*pfn)(ArgTypes...))
    {
        uintptr_t addr = FunPtr<DefaultAddr, void, ArgTypes...>::m_addr;
        auto *hook = new HookFunction<void(*)(ArgTypes...)>(addr, pfn);
        m_hook.reset(hook);
    }

    void callOrig(ArgTypes... args)
    {
        m_hook->GetTrampoline()(args...);
    }

protected:
    std::unique_ptr<HookFunction<void(*)(ArgTypes...)>> m_hook;
};

