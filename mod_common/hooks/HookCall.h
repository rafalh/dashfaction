#pragma once

#include "precomp.h"
#include "MemChange.h"

class HookCallInternal : protected MemChange
{
private:
	bool m_bIsValid;

protected:
	HookCallInternal(uintptr_t OpcodeAddr, uintptr_t ExpectedFunAddr);
	bool HookInternal(uintptr_t FunPtr);
	uintptr_t GetPrevAddr() const;
};

template <typename T>
class HookCall : private HookCallInternal
{
    public:
		HookCall(uint32_t OpcodeAddr, T ExpectedFunPtr = nullptr) :
			HookCallInternal(OpcodeAddr, reinterpret_cast<uintptr_t>(ExpectedFunPtr)) {}

		virtual ~HookCall()
		{
			Unhook();
		}

		void Unhook()
		{
			Revert();
		}

		bool Hook(T FunPtr)
        {
			return HookInternal(reinterpret_cast<uintptr_t>(FunPtr));
        }

		T GetPrevFun() const
		{
			return reinterpret_cast<T>(GetPrevAddr());
		}
};
