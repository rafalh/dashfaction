#include "precomp.h"
#include "HookCall.h"
#include "CLogger.h"

HookCallInternal::HookCallInternal(uintptr_t OpcodeAddr, uintptr_t ExpectedFunAddr) :
    MemChange(OpcodeAddr + 1)
{
    unsigned char *OpcodePtr = reinterpret_cast<unsigned char*>(OpcodeAddr);
    m_bIsValid = (*OpcodePtr == 0xE8);
    if (!m_bIsValid)
        CLogger::root().err() << "Failed to set hook at address " << std::hex << OpcodeAddr << " (opcode 0x" << std::hex << *OpcodePtr << ")";
    
	if (ExpectedFunAddr)
    {
		int32_t ExpectedOffset = static_cast<int32_t>(ExpectedFunAddr)-(reinterpret_cast<uint32_t>(GetAddr()) + 4);
        int32_t CurrentOffset;
        memcpy(&CurrentOffset, OpcodePtr+1, sizeof(CurrentOffset));
        if (CurrentOffset != ExpectedOffset)
        {
			uintptr_t CurrentFunAddr = static_cast<uintptr_t>(CurrentOffset + (reinterpret_cast<uintptr_t>(GetAddr()) + 4));
			CLogger::root().warn() << "Unexpected function address in hook at " << std::hex << OpcodeAddr << ": expected " << ExpectedFunAddr << ", got " << CurrentFunAddr;
        }
    }
}

bool HookCallInternal::HookInternal(uintptr_t FunAddr)
{
    if (!m_bIsValid)
        return false;
    
	int32_t Offset = static_cast<int32_t>(FunAddr) - (reinterpret_cast<uint32_t>(GetAddr()) + 4);
    Write(&Offset, sizeof(Offset));
    return true;
}

uintptr_t HookCallInternal::GetPrevAddr() const
{
	return *(reinterpret_cast<const uintptr_t*>(GetPrevData()));
}
