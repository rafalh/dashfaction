#pragma once

#include <subhook.h>

template<typename RetType, typename... ArgTypes>
class FunHook
{
public:
    typedef RetType(*FunPtr)(ArgTypes...);

    FunHook(FunPtr fun)
    {
        m_fun = fun;
    }

    FunHook(FunHook &&hook)
    {
        m_fun = hook.m_fun;
    }

    void hook(FunPtr newFun)
    {
        m_hook.Install(m_fun, newFun);
    }

    RetType callTrampoline(ArgTypes... args)
    {
        FunPtr trampoline = (FunPtr)m_hook.GetTrampoline();
        return trampoline(args...);
    }

private:
    FunPtr m_fun;
    subhook::Hook m_hook;
};

template<typename RetType, typename... ArgTypes>
FunHook<RetType, ArgTypes...> makeFunHook(RetType(*fun)(ArgTypes...))
{
    return FunHook<RetType, ArgTypes...>(fun);
}

#if 0
void f1(int) {}
void f2(int) {}
void example()
{
    //FunHook<decltype(RunGame)> hook1(RunGame);
    auto hook2 = makeFunHook(f1);
    hook2.hook(f2);
}
#endif
