#pragma once

#include "AsmWritter.h"
#include <cstdint>
#include <log/Logger.h>
#include <subhook.h>

#define REG_UNION(letter)     \
    union                     \
    {                         \
        int32_t e##letter##x; \
        int16_t letter##x;    \
        struct                \
        {                     \
            int8_t letter##l; \
            int8_t letter##h; \
        };                    \
    }

struct X86Regs
{
    uint32_t eflags;
    // reversed PUSHA order of registers
    int32_t edi;
    int32_t esi;
    int32_t ebp;
    int32_t reserved; // unused esp
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
    RegsPatch(uintptr_t addr, void (*fun_ptr)(X86Regs& regs)) : m_addr(addr), m_fun_ptr(fun_ptr) {}

    void Install()
    {
        void* wrapper = AllocMemForCode(256);

        m_subhook.Install(reinterpret_cast<void*>(m_addr), wrapper);
        void* trampoline = m_subhook.GetTrampoline();
        if (!trampoline)
            WARN("trampoline is null for 0x%p", m_addr);

        AsmWritter(reinterpret_cast<uintptr_t>(wrapper))
            .push(reinterpret_cast<int32_t>(trampoline)) // Push default EIP = trampoline
            .push(asm_regs::esp) // push esp before PUSHA so it can be popped manually after POPA
            .pusha()             // push general registers
            .pushf()             // push EFLAGS
            .add({true, {asm_regs::esp}, offsetof(X86Regs, esp)}, 4) // restore esp from before pushing the return address
            .push(asm_regs::esp)              // push address of X86Regs struct (handler param)
            .call(m_fun_ptr)                  // call handler
            .add({false, {asm_regs::esp}}, 4) // free handler param
            .add({true, {asm_regs::esp}, offsetof(X86Regs, esp)}, -4) // make esp value return address aware (again)
            .mov(asm_regs::eax, {true, {asm_regs::esp}, offsetof(X86Regs, eip)}) // read EIP from X86Regs
            .mov(asm_regs::ecx, {true, {asm_regs::esp}, offsetof(X86Regs, esp)}) // read esp from X86Regs
            .mov({true, {asm_regs::ecx}, 0}, asm_regs::eax) // copy EIP to new address of the stack pointer
            .popf()                                         // pop EFLAGS
            .popa()                                         // pop general registers. Note: POPA discards esp value
            .pop(asm_regs::esp)                             // pop esp manually
            .ret();                                         // return to address read from EIP field in X86Regs
    }
};

class BaseRegsPatch2
{
private:
    uintptr_t m_addr;
    void* m_fun_ptr;
    void* m_this_ptr;
    subhook::Hook m_subhook;

public:
    BaseRegsPatch2(uintptr_t addr, void* fun_ptr, void* this_ptr) :
        m_addr(addr), m_fun_ptr(fun_ptr), m_this_ptr(this_ptr)
    {}

    virtual ~BaseRegsPatch2(){};

    void Install()
    {
        void* wrapper = AllocMemForCode(256);

        m_subhook.Install(reinterpret_cast<void*>(m_addr), wrapper);
        void* trampoline = m_subhook.GetTrampoline();
        if (!trampoline)
            WARN("trampoline is null for 0x%p", m_addr);

        AsmWritter(reinterpret_cast<uintptr_t>(wrapper))
            .push(reinterpret_cast<int32_t>(trampoline)) // Push default EIP = trampoline
            .push(asm_regs::esp) // push esp before PUSHA so it can be popped manually after POPA
            .pusha()             // push general registers
            .pushf()             // push EFLAGS
            .add({true, {asm_regs::esp}, offsetof(X86Regs, esp)},
                 4)              // restore esp value from before pushing the return address
            .push(asm_regs::esp) // push address of X86Regs struct (handler param)
            .mov(asm_regs::ecx, reinterpret_cast<uint32_t>(m_this_ptr)) // save this pointer in ECX (thiscall)
            .call(m_fun_ptr) // call handler (thiscall - calee cleans the stack)
            .add({true, {asm_regs::esp}, offsetof(X86Regs, esp)}, -4) // make esp value return address aware (again)
            .mov(asm_regs::eax, {true, {asm_regs::esp}, offsetof(X86Regs, eip)}) // read EIP from X86Regs
            .mov(asm_regs::ecx, {true, {asm_regs::esp}, offsetof(X86Regs, esp)}) // read esp from X86Regs
            .mov({true, {asm_regs::ecx}, 0}, asm_regs::eax) // copy EIP to new address of the stack pointer
            .popf()                                         // pop EFLAGS
            .popa()                                         // pop general registers. Note: POPA discards esp value
            .pop(asm_regs::esp)                             // pop esp manually
            .ret();                                         // return to address read from EIP field in X86Regs
    }

    void SetAddr(uintptr_t addr)
    {
        m_addr = addr;
    }
};

template<typename T>
class RegsPatch2 : public BaseRegsPatch2
{
    T m_functor;

public:
    RegsPatch2(uintptr_t addr, T handler) :
        BaseRegsPatch2(addr, reinterpret_cast<void*>(&wrapper), this), m_functor(handler)
    {}

    static void __thiscall wrapper(RegsPatch2& self, X86Regs& regs)
    {
        self.m_functor(regs);
    }
};
