#pragma once

#include "precomp.h"
#include "HookCall.h"
#include "Revertable.h"

template <typename T>
class HookMultiCall : public Revertable
{
public:
	HookMultiCall(T ExpectedFunPtr = nullptr) :
		m_ExpectedFunPtr(ExpectedFunPtr) {}

    HookMultiCall(uintptr_t ExpectedFunAddr) :
		m_ExpectedFunPtr(reinterpret_cast<T>(ExpectedFunAddr)) {}

	HookMultiCall &AddAddr(uintptr_t Addr)
	{
		m_Hooks.push_back(std::unique_ptr<HookCall<T>>(new HookCall<T>(Addr, m_ExpectedFunPtr)));
		return *this;
	}

	void Hook(T FuncPtr)
	{
		for (const auto &pHook : m_Hooks)
			pHook->Hook(FuncPtr);
	}

	void Revert()
	{
		for (const auto &pHook : m_Hooks)
			pHook->Unhook();
	}

private:
	std::vector<std::unique_ptr<HookCall<T>>> m_Hooks;
	T m_ExpectedFunPtr;
};

