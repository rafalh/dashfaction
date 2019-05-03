#pragma once

#include <patch_common/AsmWritter.h>
#include <cstdint>
#include <log/Logger.h>
#include <subhook.h>

#define X86_GP_REG_UNION(letter) \
    union                        \
    {                            \
        int32_t e##letter##x;    \
        int16_t letter##x;       \
        struct                   \
        {                        \
            int8_t letter##l;    \
            int8_t letter##h;    \
        };                       \
    }

struct X86Regs
{
    uint32_t eflags;
    // reversed PUSHA order of registers
    int32_t edi;
    int32_t esi;
    int32_t ebp;
    int32_t reserved; // unused esp
    X86_GP_REG_UNION(b);
    X86_GP_REG_UNION(d);
    X86_GP_REG_UNION(c);
    X86_GP_REG_UNION(a);
    // real esp
    int32_t esp;
    // return address
    uint32_t eip;
};

#undef X86_GP_REG_UNION

class BaseCodeInjection
{
private:
    uintptr_t m_addr;
    subhook::Hook m_subhook;

public:
    BaseCodeInjection(uintptr_t addr) : m_addr(addr) {}
    virtual ~BaseCodeInjection() {}

    void Install()
    {
        void* wrapper = AllocMemForCode(256);

        m_subhook.Install(reinterpret_cast<void*>(m_addr), wrapper);
        void* trampoline = m_subhook.GetTrampoline();
        if (!trampoline)
            WARN("trampoline is null for 0x%X", m_addr);

        using namespace asm_regs;
        AsmWritter asm_writter{reinterpret_cast<uintptr_t>(wrapper)};
        asm_writter
            .push(reinterpret_cast<int32_t>(trampoline)) // Push default EIP = trampoline
            .push(esp)                                 // push ESP before PUSHA so it can be popped manually after POPA
            .pusha()                                   // push general registers
            .pushf()                                   // push EFLAGS
            .add(*(esp + offsetof(X86Regs, esp)), 4);  // restore esp from before pushing the return address
        EmitHandlerCall(asm_writter);                  // call handler
        asm_writter
            .add(*(esp + offsetof(X86Regs, esp)), -4)  // make esp value return address aware (again)
            .mov(eax, *(esp + offsetof(X86Regs, eip))) // read EIP from X86Regs
            .mov(ecx, *(esp + offsetof(X86Regs, esp))) // read esp from X86Regs
            .mov(*ecx, eax)                            // copy EIP to new address of the stack pointer
            .popf()                                    // pop EFLAGS
            .popa()                                    // pop general registers. Note: POPA discards esp value
            .pop(esp)                                  // pop esp manually
            .ret();                                    // return to address read from EIP field in X86Regs
    }

    void SetAddr(uintptr_t addr)
    {
        m_addr = addr;
    }

protected:
    virtual void EmitHandlerCall(AsmWritter& asm_writter) = 0;
};

class CodeInjection : public BaseCodeInjection
{
private:
    void (*m_fun_ptr)(X86Regs& regs);

public:
    CodeInjection(uintptr_t addr, void (*fun_ptr)(X86Regs& regs)) :
        BaseCodeInjection(addr), m_fun_ptr(fun_ptr) {}

protected:
    void EmitHandlerCall(AsmWritter& asm_writter) override
    {
        using namespace asm_regs;
        asm_writter
            .push(esp)        // push address of X86Regs struct (handler param)
            .call(m_fun_ptr)  // call handler (cdecl)
            .add(esp, 4);     // free handler param
    }
};

template<typename T>
class CodeInjection2 : public BaseCodeInjection
{
    T m_functor;

public:
    CodeInjection2(uintptr_t addr, T handler) :
        BaseCodeInjection(addr), m_functor(handler)
    {}

private:
    static void __thiscall wrapper(CodeInjection2& self, X86Regs& regs)
    {
        self.m_functor(regs);
    }

protected:
    void EmitHandlerCall(AsmWritter& asm_writter) override
    {
        using namespace asm_regs;
        asm_writter
            .push(esp)                                   // push address of X86Regs struct (handler param)
            .mov(ecx, reinterpret_cast<uint32_t>(this))  // save this pointer in ECX (thiscall)
            .call(reinterpret_cast<void*>(&wrapper));    // call handler (thiscall - calee cleans the stack)
    }
};
