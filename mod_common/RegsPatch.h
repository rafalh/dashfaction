#pragma once

#include <cstdint>
#include <subhook.h>
#include <log/Logger.h>
#include "AsmWritter.h"

#define REG_UNION(letter) \
    union { \
        int32_t e ## letter ## x; \
        int16_t letter ## x; \
        struct { \
            int8_t letter ## l; \
            int8_t letter ## h; \
        }; \
    }

struct X86Regs
{
    uint32_t eflags;
    // reversed PUSHA order of registers
    int32_t edi;
    int32_t esi;
    int32_t ebp;
    int32_t reserved; // unused ESP
    REG_UNION(b);
    REG_UNION(d);
    REG_UNION(c);
    REG_UNION(a);
    // real esp
    int32_t esp;
    // return address
    uint32_t eip;
};

class RegsPatch
{
private:
    uintptr_t m_addr;
    void (*m_fun_ptr)(X86Regs& regs);
    subhook::Hook m_subhook;

public:
    RegsPatch(uintptr_t addr, void (*fun_ptr)(X86Regs& regs)) :
        m_addr(addr), m_fun_ptr(fun_ptr)
    {}

    void Install()
    {
        void *wrapper = new char[256];
        UnprotectMem(wrapper, 256);

        m_subhook.Install(reinterpret_cast<void*>(m_addr), wrapper);
        void *trampoline = m_subhook.GetTrampoline();
        if (!trampoline)
            WARN("trampoline is null for 0x%p", m_addr);

        AsmWritter(reinterpret_cast<uintptr_t>(wrapper))
            .push(reinterpret_cast<int32_t>(trampoline))            // Push default EIP = trampoline
            .push(AsmRegs::ESP)                                     // push ESP before PUSHA so it can be popped manually after POPA
            .pusha()                                                // push general registers
            .pushf()                                                // push EFLAGS
            .add({true, {AsmRegs::ESP}, offsetof(X86Regs, esp)}, 4) // restore ESP value from before pushing the return address
            .push(AsmRegs::ESP)                                     // push address of X86Regs struct (handler param)
            .callLong(m_fun_ptr)                                     // call handler
            .add({false, {AsmRegs::ESP}}, 4)                        // free handler param
            .add({true, {AsmRegs::ESP}, offsetof(X86Regs, esp)}, -4) // make ESP value return address aware (again)
            .mov(AsmRegs::EAX, {true, {AsmRegs::ESP}, offsetof(X86Regs, eip)}) // read EIP from X86Regs
            .mov(AsmRegs::ECX, {true, {AsmRegs::ESP}, offsetof(X86Regs, esp)}) // read ESP from X86Regs
            .mov({true, {AsmRegs::ECX}, 0}, AsmRegs::EAX)           // copy EIP to new address of the stack pointer
            .popf()                                                 // pop EFLAGS
            .popa()                                                 // pop general registers. Note: POPA discards ESP value
            .pop(AsmRegs::ESP)                                      // pop ESP manually
            .ret();                                                 // return to address read from EIP field in X86Regs
    }
};
