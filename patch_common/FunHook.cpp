#include <patch_common/FunHook.h>
#include <xlog/xlog.h>

void FunHookImpl::Install()
{
    m_subhook.Install(m_target_fun_ptr, m_hook_fun_ptr);
    if (!m_subhook.GetTrampoline())
        xlog::error("trampoline is null for 0x%p", m_target_fun_ptr);
}
