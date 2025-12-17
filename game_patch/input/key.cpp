#include <cctype>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include "../rf/input.h"
#include "../rf/os/console.h"
#include "../rf/os/os.h"

FunHook<int(int16_t)> key_to_ascii_hook{
    0x0051EFC0,
    [](int16_t key) {
        using namespace rf;
        constexpr int empty_result = 0xFF;
        if (!key) {
            return empty_result;
        }
        // special handling for Num Lock (because ToAscii API does not support it)
        switch (key & KEY_MASK) {
            // Numpad keys that always work
            case KEY_PADMULTIPLY: return static_cast<int>('*');
            case KEY_PADMINUS: return static_cast<int>('-');
            case KEY_PADPLUS: return static_cast<int>('+');
            // Disable Numpad Enter key because game is not prepared for getting new line character from this function
            case KEY_PADENTER: return empty_result;
        }
        if (GetKeyState(VK_NUMLOCK) & 1) {
            switch (key & KEY_MASK) {
                case KEY_PAD7: return static_cast<int>('7');
                case KEY_PAD8: return static_cast<int>('8');
                case KEY_PAD9: return static_cast<int>('9');
                case KEY_PAD4: return static_cast<int>('4');
                case KEY_PAD5: return static_cast<int>('5');
                case KEY_PAD6: return static_cast<int>('6');
                case KEY_PAD1: return static_cast<int>('1');
                case KEY_PAD2: return static_cast<int>('2');
                case KEY_PAD3: return static_cast<int>('3');
                case KEY_PAD0: return static_cast<int>('0');
                case KEY_PADPERIOD: return static_cast<int>('.');
            }
        }
        BYTE key_state[256] = {0};
        if (key & KEY_SHIFTED) {
            key_state[VK_SHIFT] = 0x80;
        }
        if (key & KEY_ALTED) {
            key_state[VK_MENU] = 0x80;
        }
        if (key & KEY_CTRLED) {
            key_state[VK_CONTROL] = 0x80;
        }
        int scan_code = key & 0x7F;
        auto vk = MapVirtualKeyA(scan_code, MAPVK_VSC_TO_VK);
        WCHAR unicode_chars[3];
        auto num_unicode_chars = ToUnicode(vk, scan_code, key_state, unicode_chars, std::size(unicode_chars), 0);
        if (num_unicode_chars < 1) {
            return empty_result;
        }
        char ansi_char;
#if 0 // Windows-1252 codepage support - disabled because callers of this function expects ASCII
        int num_ansi_chars = WideCharToMultiByte(1252, 0, unicode_chars, num_unicode_chars,
            &ansi_char, sizeof(ansi_char), nullptr, nullptr);
        if (num_ansi_chars == 0) {
            return empty_result;
        }
#else
        if (static_cast<char16_t>(unicode_chars[0]) >= 0x80 || !std::isprint(unicode_chars[0])) {
            return empty_result;
        }
        ansi_char = static_cast<char>(unicode_chars[0]);
#endif
        xlog::trace("vk {:x} ({}) char {}", vk, vk, ansi_char);
        return static_cast<int>(ansi_char);
    },
};

int get_key_name(int key, char* buf, size_t buf_len)
{
     LONG lparam = (key & 0x7F) << 16;
    if (key & 0x80) {
        lparam |= 1 << 24;
    }
    // Note: it seems broken on Wine with non-US layout (most likely broken MAPVK_VSC_TO_VK_EX mapping is responsible)
    int ret = GetKeyNameTextA(lparam, buf, buf_len);
    if (ret <= 0) {
        WARN_ONCE("GetKeyNameTextA failed for 0x{:X}", key);
        buf[0] = '\0';
    }
    else {
        xlog::trace("key 0x{:x} name {}", key, buf);
    }
    return ret;
}

FunHook<int(rf::String&, int)> get_key_name_hook{
    0x0043D930,
    [](rf::String& out_name, int key) {
        static char buf[32] = "";
        int result = 0;
        if (key < 0 || get_key_name(key, buf, std::size(buf)) <= 0) {
            result = -1;
        }
        out_name = buf;
        return result;
    },
};

CodeInjection key_name_in_options_patch{
    0x00450328,
    [](auto& regs) {
        static char buf[32];
        int key = regs.edx;
        get_key_name(key, buf, std::size(buf));
        regs.edi = buf;
        regs.eip = 0x0045032F;
    },
};

CodeInjection key_get_hook{
    0x0051F000,
    []() {
        // Process messages here because when watching videos main loop is not running
        rf::os_poll();
    },
};

FunHook<void(int, int, int)> key_msg_handler_hook{
    0x0051EBA0,
    [] (const int msg, const int w_param, int l_param) {
        // For num pads, we need to fix these scan codes.
        switch (msg) {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP: {
                uint32_t scan_code = (l_param >> 16) & 0xFF;
                if (w_param == VK_PRIOR) {
                    scan_code = rf::KEY_PAGEUP;
                } else if (w_param == VK_NEXT) {
                    scan_code = rf::KEY_PAGEDOWN;
                } else if (w_param == VK_END) {
                    scan_code = rf::KEY_END;
                } else if (w_param == VK_HOME) {
                    scan_code = rf::KEY_HOME;
                }
                l_param &= 0xFF00FFFF;
                l_param |= scan_code << 16;
            }
        }
        key_msg_handler_hook.call_target(msg, w_param, l_param);
    },
};

void key_apply_patch()
{
    // Support non-US keyboard layouts
    key_to_ascii_hook.install();
    get_key_name_hook.install();
    key_name_in_options_patch.install();

    // Disable broken numlock handling
    write_mem<int8_t>(0x004B14B2 + 1, 0);

    // win32 console support and addition of os_poll
    key_get_hook.install();

    // On num pads, support `PgUp`, `PgDown`, `End`, and `Home`.
    key_msg_handler_hook.install();
}
