#include <patch_common/CallHook.h>
#include <xlog/xlog.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/MemUtils.h>
#include <cstdint>

void CallHookImpl::install()
    {
        m_target_fun_ptr = nullptr;
        for (auto addr : m_call_op_addr_vec) {
            uint8_t opcode = *reinterpret_cast<uint8_t*>(addr);
            if (opcode != asm_opcodes::call_rel_long) {
                xlog::error("not a call at 0x{:x}", addr);
                return;
            }

            intptr_t call_offset = *reinterpret_cast<intptr_t*>(addr + 1);
            int call_op_size = 1 + sizeof(uintptr_t);
            auto* call_addr = reinterpret_cast<void*>(addr + call_op_size + call_offset);
            if (!m_target_fun_ptr) {
                m_target_fun_ptr = call_addr;
            }
            else if (call_addr != m_target_fun_ptr) {
                xlog::error("call target function differs at 0x{:x}", addr);
            }

            intptr_t new_offset = reinterpret_cast<intptr_t>(m_hook_fun_ptr) - addr - call_op_size;
            write_mem<i32>(addr + 1, new_offset);
        }
    }
