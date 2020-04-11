#pragma once

#include <windef.h>
#include <patch_common/MemUtils.h>
#include "common.h"

namespace rf
{
    class CmdLineParam
    {
        using SelfType = CmdLineParam;

        struct CmdLineParam *next;
        struct CmdLineParam *prev;
        const char *name;
        void *unknown;
        char *arg;
        bool is_enabled;
        bool has_arg;

    public:
        CmdLineParam(const char* name, const char* unk, bool has_arg)
        {
            auto fun_ptr = reinterpret_cast<SelfType&(__thiscall*)(SelfType & self, const char* name, const char* unk, bool has_arg)>(0x00523690);
            fun_ptr(*this, name, unk, has_arg);
        }

        bool IsEnabled() const
        {
            return is_enabled;
        }

        const char* GetArg() const
        {
            return arg;
        }
    };

    /* User Interface (UI) */

    struct UiGadget
    {
        void **vtbl;
        UiGadget *parent;
        bool highlighted;
        bool enabled;
        int x;
        int y;
        int w;
        int h;
        int key;
        void(*on_click)();
        void(*on_mouse_btn_down)();
    };
    static_assert(sizeof(UiGadget) == 0x28);

    static auto& UiMsgBox = AddrAsRef<void(const char *title, const char *text, void(*callback)(), bool input)>(0x004560B0);
    using UiDialogCallbackPtr = void (*)();
    static auto& UiCreateDialog =
        AddrAsRef<void(const char *title, const char *text, unsigned num_buttons, const char *ppsz_btn_titles[],
                       UiDialogCallbackPtr callbacks[], unsigned unknown1, unsigned unknown2)>(0x004562A0);
    static auto& UiGetGadgetFromPos = AddrAsRef<int(int x, int y, UiGadget * const gadgets[], int num_gadgets)>(0x00442ED0);


    static auto& is_hud_hidden = AddrAsRef<bool>(0x006379F0);

    /* Chat */

    enum class ChatMsgColor
    {
        red_white = 0,
        blue_white = 1,
        red_red = 2,
        blue_blue = 3,
        white_white = 4,
        default_ = 5,
    };

    static auto& ChatPrint = AddrAsRef<void(String::Pod text, ChatMsgColor color, String::Pod prefix)>(0x004785A0);
    static auto& ChatSay = AddrAsRef<void(const char *msg, bool is_team_msg)>(0x00444150);

    /* File System */

    static auto& PackfileLoad = AddrAsRef<int(const char *file_name, const char *dir)>(0x0052C070);
    static auto& FsAddDirectoryEx = AddrAsRef<int(const char *dir, const char *ext_list, bool unknown)>(0x00514070);

    /* Window */

    typedef void(*MsgHandlerPtr)(UINT, WPARAM, LPARAM);
    static auto &AddMsgHandler = AddrAsRef<unsigned(MsgHandlerPtr)>(0x00524AE0);

    static auto& msg_handlers = AddrAsRef<MsgHandlerPtr[32]>(0x01B0D5A0);
    static auto& num_msg_handlers = AddrAsRef<uint32_t>(0x01B0D760);

    static auto& main_wnd = AddrAsRef<HWND>(0x01B0D748);
    static auto& is_main_wnd_active = AddrAsRef<uint8_t>(0x01B0D750);
    static auto& close_app_req = AddrAsRef<uint8_t>(0x01B0D758);
    static auto& num_redraw_server = AddrAsRef<uint32_t>(0x01775698);

    /* Other */

    struct RflLightmap
    {
        uint8_t *unk;
        int w;
        int h;
        uint8_t *buf;
        int bm_handle;
        int lightmap_idx;
    };
    static_assert(sizeof(RflLightmap) == 0x18);

    static auto& root_path = AddrAsRef<char[256]>(0x018060E8);
    static auto& level_name = AddrAsRef<String>(0x00645FDC);
    static auto& level_filename = AddrAsRef<String>(0x00645FE4);
    static auto& level_author = AddrAsRef<String>(0x00645FEC);
    static auto& default_player_weapon = AddrAsRef<String>(0x007C7600);
    static auto& active_cutscene = AddrAsRef<void*>(0x00645320);
    static auto& rfl_static_geometry = AddrAsRef<void*>(0x006460E8);

    static auto& RfBeep = AddrAsRef<void(unsigned u1, unsigned u2, unsigned u3, float volume)>(0x00505560);
    static auto& GetFileExt = AddrAsRef<char*(const char *path)>(0x005143F0);
    static auto& SetNextLevelFilename = AddrAsRef<void(String::Pod level_filename, String::Pod save_filename)>(0x0045E2E0);
    static auto& DemoLoadLevel = AddrAsRef<void(const char *level_filename)>(0x004CC270);
    static auto& SetCursorVisible = AddrAsRef<void(bool visible)>(0x0051E680);
    static auto& CutsceneIsActive = AddrAsRef<bool()>(0x0045BE80);
    static auto& TimerGet = AddrAsRef<int(int mult)>(0x00504AB0);
    static auto& GeomClearCache = AddrAsRef<void()>(0x004F0B90);
    static auto& FileGetChecksum = AddrAsRef<int(const char* filename)>(0x00436630);

    /* Strings Table */
    namespace strings {
        static const auto &array = AddrAsRef<char*[1000]>(0x007CBBF0);
        static const auto &player = array[675];
        static const auto &frags = array[676];
        static const auto &ping = array[677];
        static const auto &caps = array[681];
        static const auto &was_killed_by_his_own_hand = array[693];
        static const auto &was_killed_by = array[694];
        static const auto &was_killed_mysteriously = array[695];
        static const auto &score = array[720];
        static const auto &player_name = array[835];
        static const auto &exiting_game = array[884];
        static const auto &usage = array[886];
        static const auto &you_killed_yourself = array[942];
        static const auto &you_just_got_beat_down_by = array[943];
        static const auto &you_were_killed_by = array[944];
        static const auto &you_killed = array[945];
        static const auto &got_beat_down_by = array[946];
        static const auto &kicking_player = array[958];
    }
}
