#pragma once

#include "precomp.h"
#include "Revertable.h"
#include <subhook.h>

template <typename T>
class HookFunction : public Revertable
{
public:
	HookFunction(T FunPtr, T HookFunPtr)
	{
        Hook((void*)FunPtr, (void*)HookFunPtr);
	}

    HookFunction(uintptr_t FunAddr, T HookFunPtr)
    {
        Hook((void*)FunAddr, (void*)HookFunPtr);
    }

    T GetTrampoline()
    {
        return reinterpret_cast<T>(m_hook.GetTrampoline());
    }

	void Revert()
	{
        m_hook.Remove();
	}

private:
    subhook::Hook m_hook;

	void Hook(void *FunPtr, void *HookPtr)
	{
        m_hook.Install(FunPtr, HookPtr);
        if (!m_hook.GetTrampoline())
            logging::Logger::root().err() << "Trampoline is null for " << FunPtr;
	}
};

