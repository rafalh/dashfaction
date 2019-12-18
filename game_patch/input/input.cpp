// RF uses DirectInput 8
#define DIRECTINPUT_VERSION	0x0800

#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <windows.h>
#include <dinput.h>
#include <algorithm>
#include "../rf.h"
#include "../console/console.h"
#include "../main.h"

namespace rf
{
    auto& di_mouse = AddrAsRef<LPDIRECTINPUTDEVICE8A>(0x0188545C);
    auto& keep_mouse_centered = AddrAsRef<bool>(0x01885471);
}

bool SetDirectInputEnabled(bool enabled)
{
    auto direct_input_initialized = AddrAsRef<bool>(0x01885460);
    auto InitDirectInput = AddrAsRef<int()>(0x0051E070);
    rf::direct_input_disabled = !enabled;
    if (enabled && !direct_input_initialized) {
        if (InitDirectInput() != 0) {
            ERR("Failed to initialize DirectInput");
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

FunHook<void()> mouse_eval_deltas_di_hook{
    0x0051DEB0,
    []() {
        mouse_eval_deltas_di_hook.CallTarget();

        // center cursor if in game
        if (rf::keep_mouse_centered) {
            POINT pt{rf::GrGetMaxWidth() / 2, rf::GrGetMaxHeight() / 2};
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
        mouse_keep_centered_enable_hook.CallTarget();
    },
};

FunHook<void()> mouse_keep_centered_disable_hook{
    0x0051E6A0,
    []() {
        if (rf::keep_mouse_centered)
            SetDirectInputEnabled(false);
        mouse_keep_centered_disable_hook.CallTarget();
    },
};

DcCommand2 input_mode_cmd{
    "inputmode",
    []() {
        g_game_config.direct_input = !g_game_config.direct_input;
        g_game_config.save();

        if (g_game_config.direct_input) {
            if (!SetDirectInputEnabled(g_game_config.direct_input)) {
                rf::DcPrintf("Failed to initialize DirectInput");
            }
            else {
                SetDirectInputEnabled(rf::keep_mouse_centered);
                rf::DcPrintf("DirectInput is enabled");
            }
        }
        else
            rf::DcPrintf("DirectInput is disabled");
    },
    "Toggles input mode",
};

DcCommand2 ms_cmd{
    "ms",
    [](std::optional<float> value_opt) {
        if (value_opt) {
            float value = value_opt.value();
            value = std::clamp(value, 0.0f, 1.0f);
            rf::local_player->config.controls.mouse_sensitivity = value;
        }
        rf::DcPrintf("Mouse sensitivity: %.4f", rf::local_player->config.controls.mouse_sensitivity);
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

CodeInjection LinearPitchPatch{
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
        TRACE("non-lin %f lin %f delta %f new %f", current_pitch_non_lin, current_pitch_lin, pitch_delta,
              new_pitch_delta);
        pitch_delta = new_pitch_delta;
    },
};

DcCommand2 linear_pitch_cmd{
    "linear_pitch",
    []() {
#ifdef DEBUG
        LinearPitchTest();
#endif

        g_game_config.linear_pitch = !g_game_config.linear_pitch;
        g_game_config.save();
        rf::DcPrintf("Linear pitch is %s", g_game_config.linear_pitch ? "enabled" : "disabled");
    },
    "Toggles linear pitch angle",
};

void InputInit()
{
    // Fix keyboard layout
    uint8_t kbd_layout = 0;
    if (MapVirtualKeyA(0x10, MAPVK_VSC_TO_VK) == 'A')
        kbd_layout = 2; // AZERTY
    else if (MapVirtualKeyA(0x15, MAPVK_VSC_TO_VK) == 'Z')
        kbd_layout = 3; // QWERTZ
    INFO("Keyboard layout: %u", kbd_layout);
    WriteMem<u8>(0x004B14B4 + 1, kbd_layout);

    // Add DirectInput mouse support
    mouse_eval_deltas_di_hook.Install();
    mouse_keep_centered_enable_hook.Install();
    mouse_keep_centered_disable_hook.Install();

    // Do not limit the cursor to the game window if in menu (Win32 mouse)
    AsmWriter(0x0051DD7C).jmp(0x0051DD8E);

    // Use exclusive DirectInput mode so cursor cannot exit game window
    //WriteMem<u8>(0x0051E14B + 1, 5); // DISCL_EXCLUSIVE|DISCL_FOREGROUND

    // Linear vertical rotation (pitch)
    LinearPitchPatch.Install();

    // Commands
    input_mode_cmd.Register();
    ms_cmd.Register();
    linear_pitch_cmd.Register();
}
