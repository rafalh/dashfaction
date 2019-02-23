#pragma once

#include <cstdint>
#include <subhook.h>
#include <log/Logger.h>
#include "AsmWritter.h"

#define REG_UNION(letter) \
    union { \
        int32_t E ## letter ## X; \
        int16_t letter ## X; \
        struct { \
            int8_t letter ## L; \
            int8_t letter ## H; \
        }; \
    }

struct X86Regs
{
    // reversed PUSHA order of registers
    int32_t EDI;
    int32_t ESI;
    int32_t EBP;
    int32_t ESP;
    REG_UNION(B);
    REG_UNION(D);
    REG_UNION(C);
    REG_UNION(A);
    uint32_t EIP;
};

class RegsPatch
{
private:
    uintptr_t m_Addr;
    void (*m_FunPtr)(X86Regs &Regs);
    subhook::Hook m_Subhook;

public:
    RegsPatch(uintptr_t Addr, void (*FunPtr)(X86Regs &Regs)) :
        m_Addr(Addr), m_FunPtr(FunPtr)
    {}

    void Install()
    {
        char *Wrapper = new char[256];
        UnprotectMem(Wrapper, 256);

        m_Subhook.Install(reinterpret_cast<void*>(m_Addr), Wrapper);
        void *Trampoline = m_Subhook.GetTrampoline();
        if (!Trampoline)
            WARN("trampoline is null for 0x%p", m_Addr);

        AsmWritter(reinterpret_cast<uintptr_t>(Wrapper))
            .push(reinterpret_cast<int32_t>(Trampoline))
            .pusha()
            .add({true, {AsmRegs::ESP}, offsetof(X86Regs, ESP)}, 4)
            .push(AsmRegs::ESP)
            .callLong(m_FunPtr)
            .addEsp(4)
            .add({true, {AsmRegs::ESP}, offsetof(X86Regs, ESP)}, -4)
            .popa()
            .ret();
    }
};
