#include <cassert>
#include <algorithm>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../os/console.h"
#include "../rf/input.h"
#include "../rf/os/os.h"
#include "../rf/gr/gr.h"
#include "../rf/multi.h"
#include "../rf/player/player.h"
#include "../rf/entity.h"
#include "../main/main.h"

bool set_direct_input_enabled(bool enabled)
{
    auto direct_input_initialized = addr_as_ref<bool>(0x01885460);
    auto mouse_di_init = addr_as_ref<int()>(0x0051E070);
    rf::direct_input_disabled = !enabled;
    if (enabled && !direct_input_initialized) {
        if (mouse_di_init() != 0) {
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
            POINT pt{rf::gr::screen_width() / 2, rf::gr::screen_height() / 2};
            ClientToScreen(rf::main_wnd, &pt);
            SetCursorPos(pt.x, pt.y);
        }
    },
};

FunHook<void()> mouse_keep_centered_enable_hook{
    0x0051E690,
    []() {
        if (!rf::keep_mouse_centered && !rf::is_dedicated_server)
            set_direct_input_enabled(g_game_config.direct_input);
        mouse_keep_centered_enable_hook.call_target();
    },
};

FunHook<void()> mouse_keep_centered_disable_hook{
    0x0051E6A0,
    []() {
        if (rf::keep_mouse_centered)
            set_direct_input_enabled(false);
        mouse_keep_centered_disable_hook.call_target();
    },
};

ConsoleCommand2 input_mode_cmd{
    "inputmode",
    []() {
        g_game_config.direct_input = !g_game_config.direct_input;
        g_game_config.save();

        if (g_game_config.direct_input) {
            if (!set_direct_input_enabled(g_game_config.direct_input)) {
                rf::console::print("Failed to initialize DirectInput");
            }
            else {
                set_direct_input_enabled(rf::keep_mouse_centered);
                rf::console::print("DirectInput is enabled");
            }
        }
        else
            rf::console::print("DirectInput is disabled");
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
        rf::console::print("Mouse sensitivity: {:.4f}", rf::local_player->settings.controls.mouse_sensitivity);
    },
    "Sets mouse sensitivity",
    "ms <value>",
};

float scope_sensitivity_factor = 1.0f;
float scanner_sensitivity_factor = 1.0f;

constexpr float clamp_sensitivity_modifier(float modifier)
{
    constexpr const float min = 0.01f; // prevent division by zero in scope sens
    constexpr const float max = 4.0f;
    return std::clamp(modifier, min, max);
}

void patch_scope_and_scanner_sensitivity()
{
    AsmWriter{0x004309B1}.fmul<float>(AsmRegMem{&scope_sensitivity_factor});
    AsmWriter{0x004309DE}.fmul<float>(AsmRegMem{&scanner_sensitivity_factor});
}

void update_scope_sensitivity()
{
    float modifier = clamp_sensitivity_modifier(g_game_config.scope_sensitivity_modifier);
    scope_sensitivity_factor = rf::scope_sensitivity_constant * (1.0f / modifier);
}

void update_scanner_sensitivity()
{
    float modifier = clamp_sensitivity_modifier(g_game_config.scanner_sensitivity_modifier);
    scanner_sensitivity_factor = rf::scanner_sensitivity_constant * modifier;
}

ConsoleCommand2 scope_sens_cmd{
    "scope_sensitivity_modifier",
    [](std::optional<float> value_opt) {
        if (value_opt) {
            g_game_config.scope_sensitivity_modifier = value_opt.value();
            g_game_config.save();
            update_scope_sensitivity();
        }
        else {
            rf::console::print("Scope sensitivity modifier: {:.2f}",
                static_cast<float>(clamp_sensitivity_modifier(g_game_config.scope_sensitivity_modifier)));
        }

    },
    "Sets mouse sensitivity modifier used while scoped in.",
    "scope_sensitivity_modifier <value> (valid range: 0.01 - 4.00)",
};

ConsoleCommand2 scanner_sens_cmd{
    "scanner_sensitivity_modifier",
    [](std::optional<float> value_opt) {
        if (value_opt) {
            g_game_config.scanner_sensitivity_modifier = value_opt.value();
            g_game_config.save();
            update_scanner_sensitivity();            
        }
        else {
            rf::console::print(
                "Scanner sensitivity modifier: {:.2f}",
                static_cast<float>(clamp_sensitivity_modifier(g_game_config.scanner_sensitivity_modifier)));
        }
    },
    "Sets mouse sensitivity modifier used while in a rail scanner.",
    "scanner_sensitivity_modifier <value> (valid range: 0.01 - 4.00)",
};

rf::Vector3 fw_vector_from_non_linear_yaw_pitch(float yaw, float pitch)
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

float linear_pitch_from_forward_vector(const rf::Vector3& fvec)
{
    return std::asin(fvec.y);
}

rf::Vector3 fw_vector_from_linear_yaw_pitch(float yaw, float pitch)
{
    rf::Vector3 fvec;
    fvec.y = std::sin(pitch);
    fvec.x = std::cos(pitch) * std::sin(yaw);
    fvec.z = std::cos(pitch) * std::cos(yaw);
    fvec.normalize();
    return fvec;
}

float non_linear_pitch_from_fw_vector(rf::Vector3 fvec)
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
void linear_pitch_test()
{
    float yaw = 3.141592f / 4.0f;
    float pitch = 3.141592f / 4.0f;
    rf::Vector3 fvec = fw_vector_from_non_linear_yaw_pitch(yaw, pitch);
    float lin_pitch = linear_pitch_from_forward_vector(fvec);
    rf::Vector3 fvec2 = fw_vector_from_linear_yaw_pitch(yaw, lin_pitch);
    float pitch2 = non_linear_pitch_from_fw_vector(fvec2);
    assert(std::abs(pitch - pitch2) < 0.00001);
}
#endif // DEBUG

CodeInjection linear_pitch_patch{
    0x0049DEC9,
    [](auto& regs) {
        if (!g_game_config.linear_pitch)
            return;
        // Non-linear pitch value and delta from RF
        rf::Entity* entity = regs.esi;
        float current_yaw = entity->control_data.phb.y;
        float current_pitch_non_lin = entity->control_data.eye_phb.x;
        float& pitch_delta = *reinterpret_cast<float*>(regs.esp + 0x44 - 0x34);
        float& yaw_delta = *reinterpret_cast<float*>(regs.esp + 0x44 + 0x4);
        if (pitch_delta == 0)
            return;
        // Convert to linear space (see RotMatixFromEuler function at 004A0D70)
        auto fvec = fw_vector_from_non_linear_yaw_pitch(current_yaw, current_pitch_non_lin);
        float current_pitch_lin = linear_pitch_from_forward_vector(fvec);
        // Calculate new pitch in linear space
        float new_pitch_lin = current_pitch_lin + pitch_delta;
        float new_yaw = current_yaw + yaw_delta;
        // Clamp to [-pi, pi]
        constexpr float half_pi = 1.5707964f;
        new_pitch_lin = std::clamp(new_pitch_lin, -half_pi, half_pi);
        // Convert back to non-linear space
        auto fvec_new = fw_vector_from_linear_yaw_pitch(new_yaw, new_pitch_lin);
        float new_pitch_non_lin = non_linear_pitch_from_fw_vector(fvec_new);
        // Update non-linear pitch delta
        float new_pitch_delta = new_pitch_non_lin - current_pitch_non_lin;
        xlog::trace("non-lin {} lin {} delta {} new {}", current_pitch_non_lin, current_pitch_lin, pitch_delta,
              new_pitch_delta);
        pitch_delta = new_pitch_delta;
    },
};

ConsoleCommand2 linear_pitch_cmd{
    "linear_pitch",
    []() {
#ifdef DEBUG
        linear_pitch_test();
#endif

        g_game_config.linear_pitch = !g_game_config.linear_pitch;
        g_game_config.save();
        rf::console::print("Linear pitch is {}", g_game_config.linear_pitch ? "enabled" : "disabled");
    },
    "Toggles linear pitch angle",
};

void mouse_apply_patch()
{
    // Scale zoomed sensitivity
    patch_scope_and_scanner_sensitivity();
    update_scope_sensitivity();
    update_scanner_sensitivity();

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
    input_mode_cmd.register_cmd();
    ms_cmd.register_cmd();
    scope_sens_cmd.register_cmd();
    scanner_sens_cmd.register_cmd();
    linear_pitch_cmd.register_cmd();
}
