#pragma once

#include <patch_common/CodeBuffer.h>
#include <cstdint>
#include <subhook.h>
#include <utility>

class AsmWriter;

class BaseCodeInjection
{
private:
    uintptr_t m_addr;
    subhook::Hook m_subhook;
    CodeBuffer m_code_buf;

public:
    #define X86_GP_REG_X_UNION(letter) \
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
    #define X86_GP_REG_UNION(name) \
    union                 \
    {                     \
        int32_t e##name;  \
        int16_t name;     \
        int8_t name##l;   \
    }

    struct Regs
    {
        uint32_t eflags;
        // reversed PUSHA order of registers
        X86_GP_REG_UNION(di);
        X86_GP_REG_UNION(si);
        X86_GP_REG_UNION(bp);
        int32_t reserved; // unused esp
        X86_GP_REG_X_UNION(b);
        X86_GP_REG_X_UNION(d);
        X86_GP_REG_X_UNION(c);
        X86_GP_REG_X_UNION(a);
        // real esp
        X86_GP_REG_UNION(sp);
        // return address
        uint32_t eip;
    };
    #undef X86_GP_REG_X_UNION
    #undef X86_GP_REG_UNION

    BaseCodeInjection(uintptr_t addr) : m_addr(addr), m_code_buf(256) {}
    virtual ~BaseCodeInjection() {}

    void install();

    void set_addr(uintptr_t addr)
    {
        m_addr = addr;
    }

protected:
    virtual void emit_code(AsmWriter& asm_writter, void* trampoline) = 0;
};

template<typename T, typename Enable = void>
class CodeInjection2;

// CodeInjection2 specialization for handlers that take Regs struct reference
class BaseCodeInjectionWithRegsAccess : public BaseCodeInjection
{
protected:
    typedef void *WrapperPtr;

    WrapperPtr m_wrapper_ptr;

    BaseCodeInjectionWithRegsAccess(uintptr_t addr, WrapperPtr wrapper_ptr) :
        BaseCodeInjection(addr), m_wrapper_ptr(wrapper_ptr)
    {}

    void emit_code(AsmWriter& asm_writter, void* trampoline) override;
};

template<typename T>
class CodeInjection2<T, decltype(std::declval<T>()(std::declval<BaseCodeInjection::Regs&>()))>
 : public BaseCodeInjectionWithRegsAccess
{
    T m_functor;

public:
    CodeInjection2(uintptr_t addr, T handler) :
        BaseCodeInjectionWithRegsAccess(addr, reinterpret_cast<WrapperPtr>(&wrapper)),
        m_functor(handler)
    {}

private:
    static void __thiscall wrapper(CodeInjection2& self, Regs& regs)
    {
        self.m_functor(regs);
    }
};

// CodeInjection2 specialization for handlers that does not take any arguments

class BaseCodeInjectionWithoutRegsAccess : public BaseCodeInjection
{
protected:
    typedef void *WrapperPtr;

    WrapperPtr m_wrapper_ptr;

    BaseCodeInjectionWithoutRegsAccess(uintptr_t addr, WrapperPtr wrapper_ptr) :
        BaseCodeInjection(addr), m_wrapper_ptr(wrapper_ptr)
    {}

    void emit_code(AsmWriter& asm_writter, void* trampoline) override;
};

template<typename T>
class CodeInjection2<T, decltype(std::declval<T>()())> : public BaseCodeInjectionWithoutRegsAccess
{
    T m_functor;

public:
    CodeInjection2(uintptr_t addr, T handler) :
        BaseCodeInjectionWithoutRegsAccess(addr, reinterpret_cast<WrapperPtr>(&wrapper)),
        m_functor(handler)
    {}

private:
    static void __thiscall wrapper(CodeInjection2& self)
    {
        self.m_functor();
    }
};

template<typename T>
class CodeInjection : public CodeInjection2<T>
{
public:
    CodeInjection(uintptr_t addr, T handler) :
        CodeInjection2<T>(addr, handler)
    {}
};
