#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <windows.h>
#include "../rf.h"
#include "../commands.h"
#include "../main.h"

FunHook<void()> MouseUpdateDirectInput_hook{
    0x0051DEB0,
    []() {
        MouseUpdateDirectInput_hook.CallTarget();

        // center cursor
        POINT pt{rf::GrGetMaxWidth() / 2, rf::GrGetMaxHeight() / 2};
        ClientToScreen(rf::main_wnd, &pt);
        SetCursorPos(pt.x, pt.y);
    },
};

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
    return true;
}

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
    [](X86Regs& regs) {
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

DcCommand2 input_mode_cmd{
    "inputmode",
    []() {
        g_game_config.direct_input = !g_game_config.direct_input;
        g_game_config.save();
        if (!SetDirectInputEnabled(g_game_config.direct_input))
            rf::DcPrintf("Failed to initialize DirectInput");
        else if (g_game_config.direct_input)
            rf::DcPrintf("DirectInput is enabled");
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

void InputInit()
{
    // hook MouseUpdateDirectInput
    MouseUpdateDirectInput_hook.Install();

    // Initial DirectInput handling
    rf::direct_input_disabled = !g_game_config.direct_input;

    // Linear vertical rotation (pitch)
    LinearPitchPatch.Install();

    // Commands
    input_mode_cmd.Register();
    ms_cmd.Register();
    linear_pitch_cmd.Register();
}
