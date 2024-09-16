#pragma once

#include <vector>
#include <subhook.h>
#include <patch_common/CodeBuffer.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/Installable.h>
#include <xlog/xlog.h>

template<typename PreCallback, typename PostCallback>
class FunPrePostHook : public Installable
{
private:
    uintptr_t m_addr;
    PreCallback m_pre_cb;
    PostCallback m_post_cb;
    CodeBuffer m_enter_code;
    CodeBuffer m_leave_code;
    subhook::Hook m_subhook;
    std::vector<void*> m_ret_stack; // Note: it is not thread-safe

public:
    FunPrePostHook(uintptr_t addr, PreCallback pre_cb, PostCallback post_cb) :
        m_addr(addr), m_pre_cb(pre_cb), m_post_cb(post_cb),
        m_enter_code(64), m_leave_code(64)
    {}

    void install() override
    {
        m_subhook.Install(reinterpret_cast<void*>(m_addr), m_enter_code.get());
        void* trampoline = m_subhook.GetTrampoline();
        if (!trampoline) {
            xlog::error("trampoline is null for 0x{:x}", m_addr);
            m_subhook.Remove();
            return;
        }

        using namespace asm_regs;
        AsmWriter{m_enter_code}
            .push(ecx)                     // save ecx
            .push(edx)                     // save edx
            .push(this)                    // push enter_handler argument
            .call(&enter_handler)          // call enter_handler
            .add(esp, 4)                   // pop enter_handler argument
            .pop(edx)                      // restore edx
            .pop(ecx)                      // restore ecx
            .mov(eax, m_leave_code.get())  // put leave code address into eax
            .mov(*esp, eax)                // replace return address by our leave code
            .jmp(trampoline);              // call oryginal function
        AsmWriter{m_leave_code}
            .push(0)                       // make place for the return address
            .push(eax)                     // save eax
            .push(edx)                     // save edx
            .push(this)                    // push first leave_handler argument
            .call(&leave_handler)          // call leave handler that returns the final return address
            .add(esp, 4)                   // pop leave_handler argument
            .mov(*(esp + 8), eax)          // copy return address returned by leave_handler to the stack
            .pop(edx)                      // restore edx
            .pop(eax)                      // restore eax
            .ret();                        // return to the function caller
    }

private:
    static void enter_handler(FunPrePostHook& that, int, int, void* ret_addr)
    {
        that.m_ret_stack.push_back(ret_addr);
        that.m_pre_cb();
    }

    static void* leave_handler(FunPrePostHook& that)
    {
        that.m_post_cb();
        auto ret_addr = that.m_ret_stack.back();
        that.m_ret_stack.pop_back();
        return ret_addr;
    }
};

