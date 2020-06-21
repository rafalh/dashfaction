#pragma once

#include <windows.h>
#include <patch_common/MemUtils.h>

namespace rf
{
    typedef void(*MsgHandlerPtr)(UINT, WPARAM, LPARAM);
    static auto &AddMsgHandler = AddrAsRef<unsigned(MsgHandlerPtr)>(0x00524AE0);
    static auto &OsForeground = AddrAsRef<bool()>(0x00524AD0);

    static auto& msg_handlers = AddrAsRef<MsgHandlerPtr[32]>(0x01B0D5A0);
    static auto& num_msg_handlers = AddrAsRef<uint32_t>(0x01B0D760);

    static auto& main_wnd = AddrAsRef<HWND>(0x01B0D748);
    static auto& is_main_wnd_active = AddrAsRef<uint8_t>(0x01B0D750);
    static auto& close_app_req = AddrAsRef<uint8_t>(0x01B0D758);
    static auto& num_redraw_server = AddrAsRef<uint32_t>(0x01775698);
}
