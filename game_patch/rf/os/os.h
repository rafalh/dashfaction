#pragma once

#include <windows.h>
#include <patch_common/MemUtils.h>

namespace rf
{
    class CmdLineParam
    {
        using SelfType = CmdLineParam;

        SelfType *next;
        SelfType *prev;
        const char *name;
        const char *help;
        const char *arg;
        bool name_found;
        bool has_argument;

    public:
        CmdLineParam(const char* name, const char* help, bool has_argument)
        {
            AddrCaller{0x00523690}.this_call(this, name, help, has_argument);
        }

        [[nodiscard]] bool found() const
        {
            return name_found;
        }

        [[nodiscard]] const char* get_arg() const
        {
            return arg;
        }
    };

    struct CmdArg
    {
        char *arg;
        bool is_done;
    };

    using MsgHandlerPtr = void(*)(UINT, WPARAM, LPARAM);
    static auto& os_add_msg_handler = addr_as_ref<void(MsgHandlerPtr)>(0x00524AE0);
    static auto& os_foreground = addr_as_ref<bool()>(0x00524AD0);
    static auto& os_poll = addr_as_ref<void()>(0x00524B60);
    static auto& os_get_clipboard_text = addr_as_ref<void(char *buf, int max_len)>(0x00525AFC);

    static auto& msg_handlers = addr_as_ref<MsgHandlerPtr[32]>(0x01B0D5A0);
    static auto& num_msg_handlers = addr_as_ref<int>(0x01B0D760);

    static auto& main_wnd = addr_as_ref<HWND>(0x01B0D748);
    static auto& is_main_wnd_active = addr_as_ref<bool>(0x01B0D750);
    static auto& close_app_req = addr_as_ref<int>(0x01B0D758);
    static auto& console_redraw_counter = addr_as_ref<uint32_t>(0x01775698);

    static auto& lan_only_cmd_line_param = addr_as_ref<CmdLineParam>(0x0063F608);
    static auto& mod_param = addr_as_ref<CmdLineParam>(0x007D9528);

    static auto& cmdline_args = addr_as_ref<CmdArg[50]>(0x01AED368);
    static auto& cmdline_num_args = addr_as_ref<int>(0x01AED514);

    static auto& debug_error = addr_as_ref<void __cdecl(const char *filename, int line, const char *text)>(0x0050BA90);
    #define RF_DEBUG_ERROR(text) rf::debug_error(__FILE__, __LINE__, text)
}
