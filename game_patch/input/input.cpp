#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ShortTypes.h>
#include <xlog/xlog.h>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cassert>
#include "../rf/os.h"
#include "../rf/graphics.h"
#include "../rf/multi.h"
#include "../rf/player.h"
#include "../rf/input.h"
#include "../console/console.h"
#include "../main.h"

bool SetDirectInputEnabled(bool enabled)
{
    auto direct_input_initialized = addr_as_ref<bool>(0x01885460);
    auto InitDirectInput = addr_as_ref<int()>(0x0051E070);
    rf::direct_input_disabled = !enabled;
    if (enabled && !direct_input_initialized) {
        if (InitDirectInput() != 0) {
            xlog::error("Failed to initialize DirectInput");
            rf::direct_input_disabled = true;
            return false;
        }
    }
    if (direct_input_initialized) {
        if (rf::direct_input_disabled)
            rf::di_mouse->Unacquire();
        else
            rf::di_mouse->Acquire();
    }

    return true;
}

FunHook<void()> mouse_eval_deltas_hook{
    0x0051DC70,
    []() {
        // disable mouse when window is not active
        if (rf::os_foreground()) {
            mouse_eval_deltas_hook.call_target();
        }
    },
};

FunHook<void()> mouse_eval_deltas_di_hook{
    0x0051DEB0,
    []() {
        mouse_eval_deltas_di_hook.call_target();

        // center cursor if in game
        if (rf::keep_mouse_centered) {
            POINT pt{rf::gr_screen_width() / 2, rf::gr_screen_height() / 2};
            ClientToScreen(rf::main_wnd, &pt);
            SetCursorPos(pt.x, pt.y);
        }
    },
};

FunHook<void()> mouse_keep_centered_enable_hook{
    0x0051E690,
    []() {
        if (!rf::keep_mouse_centered && !rf::is_dedicated_server)
            SetDirectInputEnabled(g_game_config.direct_input);
        mouse_keep_centered_enable_hook.call_target();
    },
};

FunHook<void()> mouse_keep_centered_disable_hook{
    0x0051E6A0,
    []() {
        if (rf::keep_mouse_centered)
            SetDirectInputEnabled(false);
        mouse_keep_centered_disable_hook.call_target();
    },
};

ConsoleCommand2 input_mode_cmd{
    "inputmode",
    []() {
        g_game_config.direct_input = !g_game_config.direct_input;
        g_game_config.save();

        if (g_game_config.direct_input) {
            if (!SetDirectInputEnabled(g_game_config.direct_input)) {
                rf::console_printf("Failed to initialize DirectInput");
            }
            else {
                SetDirectInputEnabled(rf::keep_mouse_centered);
                rf::console_printf("DirectInput is enabled");
            }
        }
        else
            rf::console_printf("DirectInput is disabled");
    },
    "Toggles input mode",
};

ConsoleCommand2 ms_cmd{
    "ms",
    [](std::optional<float> value_opt) {
        if (value_opt) {
            float value = value_opt.value();
            value = std::clamp(value, 0.0f, 1.0f);
            rf::local_player->settings.controls.mouse_sensitivity = value;
        }
        rf::console_printf("Mouse sensitivity: %.4f", rf::local_player->settings.controls.mouse_sensitivity);
    },
    "Sets mouse sensitivity",
    "ms <value>",
};

rf::Vector3 ForwardVectorFromNonLinearYawPitch(float yaw, float pitch)
{
    // Based on RF code
    rf::Vector3 fvec0;
    fvec0.y = std::sin(pitch);
    float factor = 1.0f - std::abs(fvec0.y);
    fvec0.x = factor * std::sin(yaw);
    fvec0.z = factor * std::cos(yaw);

    rf::Vector3 fvec = fvec0;
    fvec.normalize(); // vector is never zero

    return fvec;
}

float LinearPitchFromForwardVector(const rf::Vector3& fvec)
{
    return std::asin(fvec.y);
}

rf::Vector3 ForwardVectorFromLinearYawPitch(float yaw, float pitch)
{
    rf::Vector3 fvec;
    fvec.y = std::sin(pitch);
    fvec.x = std::cos(pitch) * std::sin(yaw);
    fvec.z = std::cos(pitch) * std::cos(yaw);
    fvec.normalize();
    return fvec;
}

float NonLinearPitchFromForwardVector(rf::Vector3 fvec)
{
    float yaw = std::atan2(fvec.x, fvec.z);
    assert(!std::isnan(yaw));
    float fvec_y_2 = fvec.y * fvec.y;
    float y_sin = std::sin(yaw);
    float y_cos = std::cos(yaw);
    float y_sin_2 = y_sin * y_sin;
    float y_cos_2 = y_cos * y_cos;
    float p_sgn = std::signbit(fvec.y) ? -1.f : 1.f;
    if (fvec.y == 0.0f) {
        return 0.0f;
    }

    float a = 1.f / fvec_y_2 - y_sin_2 - 1.f - y_cos_2;
    float b = 2.f * p_sgn * y_sin_2 + 2.f * p_sgn * y_cos_2;
    float c = -y_sin_2 - y_cos_2;
    float delta = b * b - 4.f * a * c;
    // Note: delta is sometimes slightly below 0 - most probably because of precision error
    // To avoid NaN value delta is changed to 0 in that case
    float delta_sqrt = std::sqrt(std::max(delta, 0.0f));
    assert(!std::isnan(delta_sqrt));

    if (a == 0.0f) {
        return 0.0f;
    }

    float p_sin_1 = (-b - delta_sqrt) / (2.f * a);
    float p_sin_2 = (-b + delta_sqrt) / (2.f * a);

    float result;
    if (std::abs(p_sin_1) < std::abs(p_sin_2))
        result = std::asin(p_sin_1);
    else
        result = std::asin(p_sin_2);
    assert(!std::isnan(result));
    return result;
}

#ifdef DEBUG
void LinearPitchTest()
{
    float yaw = 3.141592f / 4.0f;
    float pitch = 3.141592f / 4.0f;
    rf::Vector3 fvec = ForwardVectorFromNonLinearYawPitch(yaw, pitch);
    float lin_pitch = LinearPitchFromForwardVector(fvec);
    rf::Vector3 fvec2 = ForwardVectorFromLinearYawPitch(yaw, lin_pitch);
    float pitch2 = NonLinearPitchFromForwardVector(fvec2);
    assert(std::abs(pitch - pitch2) < 0.00001);
}
#endif // DEBUG

CodeInjection linear_pitch_patch{
    0x0049DEC9,
    [](auto& regs) {
        if (!g_game_config.linear_pitch)
            return;
        // Non-linear pitch value and delta from RF
        float& current_yaw = *reinterpret_cast<float*>(regs.esi + 0x868);
        float& current_pitch_non_lin = *reinterpret_cast<float*>(regs.esi + 0x87C);
        float& pitch_delta = *reinterpret_cast<float*>(regs.esp + 0x44 - 0x34);
        float& yaw_delta = *reinterpret_cast<float*>(regs.esp + 0x44 + 0x4);
        if (pitch_delta == 0)
            return;
        // Convert to linear space (see RotMatixFromEuler function at 004A0D70)
        auto fvec = ForwardVectorFromNonLinearYawPitch(current_yaw, current_pitch_non_lin);
        float current_pitch_lin = LinearPitchFromForwardVector(fvec);
        // Calculate new pitch in linear space
        float new_pitch_lin = current_pitch_lin + pitch_delta;
        float new_yaw = current_yaw + yaw_delta;
        // Clamp to [-pi, pi]
        constexpr float half_pi = 1.5707964f;
        new_pitch_lin = std::clamp(new_pitch_lin, -half_pi, half_pi);
        // Convert back to non-linear space
        auto fvec_new = ForwardVectorFromLinearYawPitch(new_yaw, new_pitch_lin);
        float new_pitch_non_lin = NonLinearPitchFromForwardVector(fvec_new);
        // Update non-linear pitch delta
        float new_pitch_delta = new_pitch_non_lin - current_pitch_non_lin;
        xlog::trace("non-lin %f lin %f delta %f new %f", current_pitch_non_lin, current_pitch_lin, pitch_delta,
              new_pitch_delta);
        pitch_delta = new_pitch_delta;
    },
};

ConsoleCommand2 linear_pitch_cmd{
    "linear_pitch",
    []() {
#ifdef DEBUG
        LinearPitchTest();
#endif

        g_game_config.linear_pitch = !g_game_config.linear_pitch;
        g_game_config.save();
        rf::console_printf("Linear pitch is %s", g_game_config.linear_pitch ? "enabled" : "disabled");
    },
    "Toggles linear pitch angle",
};

FunHook<int(int16_t)> key_to_ascii_hook{
    0x0051EFC0,
    [](int16_t key) {
        constexpr int empty_result = 0xFF;
        if (!key) {
            return empty_result;
        }
        // special handling for Num Lock (because ToAscii API does not support it)
        switch (key & rf::KEY_MASK) {
            // Numpad keys that always work
            case rf::KEY_PADMULTIPLY: return static_cast<int>('*');
            case rf::KEY_PADMINUS: return static_cast<int>('-');
            case rf::KEY_PADPLUS: return static_cast<int>('+');
            // Disable Numpad Enter key because game is not prepared for getting new line character from this function
            case rf::KEY_PADENTER: return empty_result;
        }
        if (GetKeyState(VK_NUMLOCK) & 1) {
            switch (key & rf::KEY_MASK) {
                case rf::KEY_PAD7: return static_cast<int>('7');
                case rf::KEY_PAD8: return static_cast<int>('8');
                case rf::KEY_PAD9: return static_cast<int>('9');
                case rf::KEY_PAD4: return static_cast<int>('4');
                case rf::KEY_PAD5: return static_cast<int>('5');
                case rf::KEY_PAD6: return static_cast<int>('6');
                case rf::KEY_PAD1: return static_cast<int>('1');
                case rf::KEY_PAD2: return static_cast<int>('2');
                case rf::KEY_PAD3: return static_cast<int>('3');
                case rf::KEY_PAD0: return static_cast<int>('0');
                case rf::KEY_PADPERIOD: return static_cast<int>('.');
            }
        }
        BYTE key_state[256] = {0};
        if (key & rf::KEY_SHIFTED) {
            key_state[VK_SHIFT] = 0x80;
        }
        if (key & rf::KEY_ALTED) {
            key_state[VK_MENU] = 0x80;
        }
        if (key & rf::KEY_CTRLED) {
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
        xlog::trace("vk %X (%c) char %c", vk, vk, ansi_char);
        return static_cast<int>(ansi_char);
    },
};

int GetKeyName(int key, char* buf, size_t buf_len)
{
     LONG lparam = (key & 0x7F) << 16;
    if (key & 0x80) {
        lparam |= 1 << 24;
    }
    // Note: it seems broken on Wine with non-US layout (most likely broken MAPVK_VSC_TO_VK_EX mapping is responsible)
    int ret = GetKeyNameTextA(lparam, buf, buf_len);
    if (ret <= 0) {
        WARN_ONCE("GetKeyNameTextA failed for 0x%X", key);
        buf[0] = '\0';
    }
    else {
        xlog::trace("key 0x%X name %s", key, buf);
    }
    return ret;
}

FunHook<int(rf::String&, int)> get_key_name_hook{
    0x0043D930,
    [](rf::String& out_name, int key) {
        static char buf[32] = "";
        int result = 0;
        if (key < 0 || GetKeyName(key, buf, std::size(buf)) <= 0) {
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
        GetKeyName(key, buf, std::size(buf));
        regs.edi = reinterpret_cast<int>(buf);
        regs.eip = 0x0045032F;
    },
};

auto& GetTextFromClipboard = addr_as_ref<void(char *buf, unsigned int max_len)>(0x00525AFC);
auto UiInputBox_AddChar = reinterpret_cast<bool (__thiscall*)(void *this_, char c)>(0x00457260);

extern FunHook<bool __fastcall(void *this_, void* edx, rf::Key key)> UiInputBox_ProcessKey_hook;
bool __fastcall UiInputBox_ProcessKey_new(void *this_, void* edx, rf::Key key)
{
    if (key == (rf::KEY_V | rf::KEY_CTRLED)) {
        char buf[256];
        GetTextFromClipboard(buf, std::size(buf) - 1);
        for (int i = 0; buf[i]; ++i) {
            UiInputBox_AddChar(this_, buf[i]);
        }
        return true;
    }
    else {
        return UiInputBox_ProcessKey_hook.call_target(this_, edx, key);
    }
}
FunHook<bool __fastcall(void *this_, void* edx, rf::Key key)> UiInputBox_ProcessKey_hook{
    0x00457300,
    UiInputBox_ProcessKey_new,
};

void InputInit()
{
    // Support non-US keyboard layouts
    key_to_ascii_hook.install();
    get_key_name_hook.install();
    key_name_in_options_patch.install();

    // Disable broken numlock handling
    write_mem<u8>(0x004B14B2 + 1, 0);

    // Handle CTRL+V in input boxes
    UiInputBox_ProcessKey_hook.install();

    // Disable mouse when window is not active
    mouse_eval_deltas_hook.install();

    // Add DirectInput mouse support
    mouse_eval_deltas_di_hook.install();
    mouse_keep_centered_enable_hook.install();
    mouse_keep_centered_disable_hook.install();

    // Do not limit the cursor to the game window if in menu (Win32 mouse)
    AsmWriter(0x0051DD7C).jmp(0x0051DD8E);

    // Use exclusive DirectInput mode so cursor cannot exit game window
    //write_mem<u8>(0x0051E14B + 1, 5); // DISCL_EXCLUSIVE|DISCL_FOREGROUND

    // Linear vertical rotation (pitch)
    linear_pitch_patch.install();

    // Commands
    input_mode_cmd.Register();
    ms_cmd.Register();
    linear_pitch_cmd.Register();
}
