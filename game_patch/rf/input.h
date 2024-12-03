#pragma once

// RF uses DirectInput 8
#define DIRECTINPUT_VERSION	0x0800

#include <dinput.h>
#include <patch_common/MemUtils.h>

namespace rf
{
    enum Key
    {
        KEY_ESC = 0x1,
        KEY_1 = 0x2,
        KEY_2 = 0x3,
        KEY_3 = 0x4,
        KEY_4 = 0x5,
        KEY_5 = 0x6,
        KEY_6 = 0x7,
        KEY_7 = 0x8,
        KEY_8 = 0x9,
        KEY_9 = 0xA,
        KEY_0 = 0xB,
        KEY_MINUS = 0xC,
        KEY_EQUAL = 0xD,
        KEY_BACKSP = 0xE,
        KEY_TAB = 0xF,
        KEY_Q = 0x10,
        KEY_W = 0x11,
        KEY_E = 0x12,
        KEY_R = 0x13,
        KEY_T = 0x14,
        KEY_Y = 0x15,
        KEY_U = 0x16,
        KEY_I = 0x17,
        KEY_O = 0x18,
        KEY_P = 0x19,
        KEY_LBRACKET = 0x1A,
        KEY_RBRACKET = 0x1B,
        KEY_ENTER = 0x1C,
        KEY_LCTRL = 0x1D,
        KEY_A = 0x1E,
        KEY_S = 0x1F,
        KEY_D = 0x20,
        KEY_F = 0x21,
        KEY_G = 0x22,
        KEY_H = 0x23,
        KEY_J = 0x24,
        KEY_K = 0x25,
        KEY_L = 0x26,
        KEY_SEMICOL = 0x27,
        KEY_RAPOSTRO = 0x28,
        KEY_LAPOSTRO_DBG = 0x29,
        KEY_LSHIFT = 0x2A,
        KEY_SLASH = 0x2B,
        KEY_Z = 0x2C,
        KEY_X = 0x2D,
        KEY_C = 0x2E,
        KEY_V = 0x2F,
        KEY_B = 0x30,
        KEY_N = 0x31,
        KEY_M = 0x32,
        KEY_COMMA = 0x33,
        KEY_PERIOD = 0x34,
        KEY_DIVIDE = 0x35,
        KEY_RSHIFT = 0x36,
        KEY_PADMULTIPLY = 0x37,
        KEY_LALT = 0x38,
        KEY_SPACEBAR = 0x39,
        KEY_CAPSLOCK = 0x3A,
        KEY_F1 = 0x3B,
        KEY_F2 = 0x3C,
        KEY_F3 = 0x3D,
        KEY_F4 = 0x3E,
        KEY_F5 = 0x3F,
        KEY_F6 = 0x40,
        KEY_F7 = 0x41,
        KEY_F8 = 0x42,
        KEY_F9 = 0x43,
        KEY_F10 = 0x44,
        KEY_PAUSE = 0x45,
        KEY_SCROLLLOCK = 0x46,
        KEY_PAD7 = 0x47,
        KEY_PAD8 = 0x48,
        KEY_PAD9 = 0x49,
        KEY_PADMINUS = 0x4A,
        KEY_PAD4 = 0x4B,
        KEY_PAD5 = 0x4C,
        KEY_PAD6 = 0x4D,
        KEY_PADPLUS = 0x4E,
        KEY_PAD1 = 0x4F,
        KEY_PAD2 = 0x50,
        KEY_PAD3 = 0x51,
        KEY_PAD0 = 0x52,
        KEY_PADPERIOD = 0x53,
        KEY_F11 = 0x57,
        KEY_F12 = 0x58,
        KEY_PADENTER = 0x9C,
        KEY_RCTRL = 0x9D,
        KEY_PRINT_SCRN = 0xB7,
        KEY_RALT = 0xB8,
        KEY_NUMLOCK = 0xC5,
        KEY_BREAK = 0xC6,
        KEY_HOME = 0xC7,
        KEY_UP = 0xC8,
        KEY_PAGEUP = 0xC9,
        KEY_LEFT = 0xCB,
        KEY_END = 0xCF,
        KEY_RIGHT = 0xCD,
        KEY_DOWN = 0xD0,
        KEY_PAGEDOWN = 0xD1,
        KEY_INSERT = 0xD2,
        KEY_DELETE = 0xD3,
        KEY_MASK = 0xFF,
        KEY_SHIFTED = 0x1000,
        KEY_ALTED = 0x2000,
        KEY_CTRLED = 0x4000,
    };

    static auto& mouse_get_pos = addr_as_ref<int(int &x, int &y, int &z)>(0x0051E450);
    static auto& mouse_was_button_pressed = addr_as_ref<int(int btn_idx)>(0x0051E5D0);
    static auto& mouse_set_visible = addr_as_ref<void(bool visible)>(0x0051E680);
    static auto& key_process_event = addr_as_ref<void(int scan_code, int key_down, int delta_time)>(0x0051E6C0);

    static auto& scope_sensitivity_constant = addr_as_ref<float>(0x005895C0);
    static auto& scanner_sensitivity_constant = addr_as_ref<float>(0x005893D4);
    static auto& mouse_initialized = addr_as_ref<uint8_t>(0x01885461);
    static auto& direct_input_disabled = addr_as_ref<bool>(0x005A4F88);
    static auto& di_mouse = addr_as_ref<LPDIRECTINPUTDEVICE8A>(0x0188545C);
    static auto& keep_mouse_centered = addr_as_ref<bool>(0x01885471);
}
