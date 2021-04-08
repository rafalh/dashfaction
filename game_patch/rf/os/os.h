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

        bool found() const
        {
            return name_found;
        }

        const char* get_arg() const
        {
            return arg;
        }
    };

    typedef void(*MsgHandlerPtr)(UINT, WPARAM, LPARAM);
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
}
