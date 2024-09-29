#include <patch_common/FunHook.h>
#include <xlog/xlog.h>

void FunHookImpl::install()
{
    m_subhook.Install(m_target_fun_ptr, m_hook_fun_ptr);
    m_trampoline_ptr = m_subhook.GetTrampoline();
    if (!m_trampoline_ptr)
        xlog::error("trampoline is null for 0x{}", m_target_fun_ptr);
}
