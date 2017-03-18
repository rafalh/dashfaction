#pragma once

#include <cstdint>
#include "AsmOpcodes.h"
#include "log/Logger.h"
#include "hooks/MemChange.h"

template<typename RetType, typename... ArgTypes>
class CallHook
{
public:
    typedef RetType(*FunPtr)(ArgTypes...);

    /*CallHook(uintptr_t addr) :
        MemChange(addr + 1)
    {
        m_addr = addr;
    }

    CallHook(CallHook &&hook) :
        MemChange(hook.m_addr + 1)
    {
        m_addr = hook.m_addr;
    }*/

    void hook(uintptr_t addr, FunPtr newFun)
    {
        m_addr = addr;
        uint8_t *code = (uint8_t*)m_addr;
        if (code[0] != ASM_LONG_CALL_REL)
            ERR("Invalid opcode for call hook %x", code[0]);

        intptr_t currentOffset;
        memcpy(&currentOffset, code + 1, sizeof(currentOffset));
        uintptr_t funAddr = m_addr + 5 + currentOffset;
        m_parentFun = (FunPtr)funAddr;

        intptr_t newOffset = (intptr_t)newFun - (m_addr + 5);
        m_change.reset(new MemChange(m_addr + 1));
        m_change->Write(newOffset);
    }

    RetType callParent(ArgTypes... args)
    {
        return m_parentFun(args...);
    }

private:
    uintptr_t m_addr;
    FunPtr m_parentFun;
    std::unique_ptr<MemChange> m_change;
};

template<typename RetType, typename... ArgTypes>
CallHook<RetType, ArgTypes...> makeCallHook(RetType(*fun)(ArgTypes...))
{
    return CallHook<RetType, ArgTypes...>();
}
