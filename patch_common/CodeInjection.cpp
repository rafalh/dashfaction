#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>

void BaseCodeInjection::install()
{
    void* trampoline = nullptr;
    if (m_needs_trampoline) {
        m_subhook.Install(reinterpret_cast<void*>(m_addr), m_code_buf);
        trampoline = m_subhook.GetTrampoline();
        if (!trampoline)
            xlog::warn("trampoline is null for 0x{:x}", m_addr);
    }
    else {
        AsmWriter{m_addr}.jmp(m_code_buf.get());
    }

    AsmWriter asm_writter{m_code_buf};
    emit_code(asm_writter, trampoline);
}

void BaseCodeInjectionWithRegsAccess::emit_code(AsmWriter& asm_writter, void* trampoline)
{
    using namespace asm_regs;
    constexpr int esp_offset = offsetof(Regs, esp);
    constexpr int eip_offset = offsetof(Regs, eip);
    asm_writter
        .push(trampoline)               // Push default EIP = trampoline
        .push(esp)                      // push ESP before PUSHA so it can be popped manually after POPA
        .pusha()                        // push general registers
        .pushf()                        // push EFLAGS
        .add(*(esp + esp_offset), 4)    // restore ESP from before pushing the return address
        .push(esp)                      // push address of Regs struct (handler param)
        .mov(ecx, this)                 // save this pointer in ECX (thiscall)
        .call(m_wrapper_ptr)            // call handler (thiscall - calee cleans the stack)
        .add(*(esp + esp_offset), -4)   // make ESP value return address aware (again)
        .mov(eax, *(esp + eip_offset))  // read EIP from Regs struct
        .mov(ecx, *(esp + esp_offset))  // read ESP from Regs struct
        .mov(*ecx, eax)                 // copy EIP to new address of the stack pointer
        .popf()                         // pop EFLAGS
        .popa()                         // pop general registers. Note: POPA discards ESP value
        .pop(esp)                       // pop ESP pushed before PUSHA
        .ret();                         // return to address read from EIP field in Regs struct
}

void BaseCodeInjectionWithoutRegsAccess::emit_code(AsmWriter& asm_writter, void* trampoline)
{
    using namespace asm_regs;
    asm_writter
        .push(eax)         // push caller-saved general purpose registers
        .push(ecx)         // push caller-saved general purpose registers
        .push(edx)         // push caller-saved general purpose registers
        .pushf()           // push EFLAGS
        .mov(ecx, this)    // save this pointer in ECX (thiscall)
        .call(m_wrapper_ptr)  // call handler (thiscall - calee cleans the stack)
        .popf()            // pop EFLAGS
        .pop(edx)          // pop caller-saved general purpose registers
        .pop(ecx)          // pop caller-saved general purpose registers
        .pop(eax)          // pop caller-saved general purpose registers
        .jmp(trampoline);  // jump to the trampoline
}
