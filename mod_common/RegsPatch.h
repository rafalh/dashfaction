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
    uint32_t Flags;
    // reversed PUSHA order of registers
    int32_t EDI;
    int32_t ESI;
    int32_t EBP;
    int32_t _reserved; // unused ESP
    REG_UNION(B);
    REG_UNION(D);
    REG_UNION(C);
    REG_UNION(A);
    // real ESP
    int32_t ESP;
    // return address
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
        void *Wrapper = new char[256];
        UnprotectMem(Wrapper, 256);

        m_Subhook.Install(reinterpret_cast<void*>(m_Addr), Wrapper);
        void *Trampoline = m_Subhook.GetTrampoline();
        if (!Trampoline)
            WARN("trampoline is null for 0x%p", m_Addr);

        AsmWritter(reinterpret_cast<uintptr_t>(Wrapper))
            .push(reinterpret_cast<int32_t>(Trampoline))            // Push default EIP = trampoline
            .push(AsmRegs::ESP)                                     // push ESP before PUSHA so it can be popped manually after POPA
            .pusha()                                                // push general registers
            .pushf()                                                // push EFLAGS
            .add({true, {AsmRegs::ESP}, offsetof(X86Regs, ESP)}, 4) // restore ESP value from before pushing the return address
            .push(AsmRegs::ESP)                                     // push address of X86Regs struct (handler param)
            .callLong(m_FunPtr)                                     // call handler
            .add({false, {AsmRegs::ESP}}, 4)                        // free handler param
            .add({true, {AsmRegs::ESP}, offsetof(X86Regs, ESP)}, -4) // make ESP value return address aware (again)
            .mov(AsmRegs::EAX, {true, {AsmRegs::ESP}, offsetof(X86Regs, EIP)}) // read EIP from X86Regs
            .mov(AsmRegs::ECX, {true, {AsmRegs::ESP}, offsetof(X86Regs, ESP)}) // read ESP from X86Regs
            .mov({true, {AsmRegs::ECX}, 0}, AsmRegs::EAX)           // copy EIP to new address of the stack pointer
            .popf()                                                 // pop EFLAGS
            .popa()                                                 // pop general registers. Note: POPA discards ESP value
            .pop(AsmRegs::ESP)                                      // pop ESP manually
            .ret();                                                 // return to address read from EIP field in X86Regs
    }
};
