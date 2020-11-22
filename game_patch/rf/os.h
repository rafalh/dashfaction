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

        bool IsEnabled() const
        {
            return name_found;
        }

        const char* GetArg() const
        {
            return arg;
        }
    };

    typedef void(*MsgHandlerPtr)(UINT, WPARAM, LPARAM);
    static auto &OsAddMsgHandler = AddrAsRef<unsigned(MsgHandlerPtr)>(0x00524AE0);
    static auto &OsForeground = AddrAsRef<bool()>(0x00524AD0);

    static auto& msg_handlers = AddrAsRef<MsgHandlerPtr[32]>(0x01B0D5A0);
    static auto& num_msg_handlers = AddrAsRef<uint32_t>(0x01B0D760);

    static auto& main_wnd = AddrAsRef<HWND>(0x01B0D748);
    static auto& is_main_wnd_active = AddrAsRef<uint8_t>(0x01B0D750);
    static auto& close_app_req = AddrAsRef<uint8_t>(0x01B0D758);
    static auto& num_redraw_server = AddrAsRef<uint32_t>(0x01775698);

    static auto& lan_only_cmd_line_param = AddrAsRef<CmdLineParam>(0x0063F608);
}
