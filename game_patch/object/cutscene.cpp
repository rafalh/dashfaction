#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <algorithm>
#include <xlog/xlog.h>
#include "../console/console.h"
#include "../rf/player.h"
#include "../rf/graphics.h"
#include "../rf/hud.h"
#include "../rf/misc.h"
#include "../main.h"
#include "../misc/sound.h"

static constexpr rf::ControlAction default_skip_cutscene_ctrl = rf::CA_MP_STATS;

rf::String get_game_ctrl_bind_name(int game_ctrl)
{
    auto get_key_name = addr_as_ref<int(rf::String *out, int key)>(0x0043D930);
    auto get_mouse_button_name = addr_as_ref<int(rf::String *out, int mouse_btn)>(0x0043D970);
    auto ctrl_config = rf::local_player->settings.controls.keys[game_ctrl];
    rf::String name;
    if (ctrl_config.scan_codes[0] >= 0) {
        get_key_name(&name, ctrl_config.scan_codes[0]);
    }
    else if (ctrl_config.mouse_btn_id >= 0) {
        get_mouse_button_name(&name, ctrl_config.mouse_btn_id);
    }
    else {
        return rf::String::format("?");
    }
    return name;
}

void render_skip_cutscene_hint_text(rf::ControlAction ctrl)
{
    if (rf::is_hud_hidden) {
        return;
    }
    auto bind_name = get_game_ctrl_bind_name(ctrl);
    auto& ctrl_name = rf::local_player->settings.controls.keys[ctrl].name;
    rf::gr_set_color_rgba(255, 255, 255, 255);
    auto msg = rf::String::format("Press %s (%s) to skip the cutscene", ctrl_name.c_str(), bind_name.c_str());
    auto x = rf::gr_screen_width() / 2;
    auto y = rf::gr_screen_height() - 30;
    rf::gr_string_aligned(rf::GR_ALIGN_CENTER, x, y, msg.c_str(), -1, rf::gr_string_mode);
}

FunHook<void(bool)> cutscene_do_frame_hook{
    0x0045B5E0,
    [](bool dlg_open) {
        bool skip_cutscene = false;
        auto skip_cutscene_ctrl = g_game_config.skip_cutscene_ctrl.value() != -1
            ? static_cast<rf::ControlAction>(g_game_config.skip_cutscene_ctrl.value())
            : default_skip_cutscene_ctrl;
        rf::control_config_check_pressed(&rf::local_player->settings.controls, skip_cutscene_ctrl, &skip_cutscene);

        if (!skip_cutscene) {
            cutscene_do_frame_hook.call_target(dlg_open);
            render_skip_cutscene_hint_text(skip_cutscene_ctrl);
        }
        else {
            auto& timer_add_delta_time = addr_as_ref<int(int delta_ms)>(0x004FA2D0);

            auto& timer_base = addr_as_ref<int64_t>(0x01751BF8);
            auto& timer_freq = addr_as_ref<int32_t>(0x01751C04);
            auto& frame_time = addr_as_ref<float>(0x005A4014);
            auto& num_shots = struct_field_ref<int>(rf::active_cutscene, 4);
            auto& current_shot_idx = struct_field_ref<int>(rf::active_cutscene, 0x808);
            auto& current_shot_timer = struct_field_ref<rf::Timestamp>(rf::active_cutscene, 0x810);

            disable_sound_before_cutscene_skip();

            while (rf::CutsceneIsActive()) {
                int shot_time_left_ms = current_shot_timer.time_until();

                if (current_shot_idx == num_shots - 1) {
                    // run last half second with a speed of 10 FPS so all events get properly processed before
                    // going back to normal gameplay
                    if (shot_time_left_ms > 500)
                        shot_time_left_ms -= 500;
                    else
                        shot_time_left_ms = std::min(shot_time_left_ms, 100);
                }
                timer_add_delta_time(shot_time_left_ms);
                frame_time = shot_time_left_ms / 1000.0f;
                timer_base -= static_cast<int64_t>(shot_time_left_ms) * timer_freq / 1000;
                cutscene_do_frame_hook.call_target(dlg_open);
            }

            enable_sound_after_cutscene_skip();
        }
    },
};

CallHook<bool(rf::Camera*)> cutscene_stop_current_camera_set_first_person_hook{
    0x0045BDBD,
    [](rf::Camera* camera) {
        if (!cutscene_stop_current_camera_set_first_person_hook.call_target(camera)) {
            rf::camera_set_fixed(camera);
        }
        return true;
    },
};

ConsoleCommand2 skip_cutscene_bind_cmd{
    "skip_cutscene_bind",
    [](std::string bind_name) {
        if (bind_name == "default") {
            g_game_config.skip_cutscene_ctrl = -1;
        }
        else {
            auto ConfigFindControlByName = addr_as_ref<int(rf::PlayerSettings&, const char*)>(0x0043D9F0);
            int ctrl = ConfigFindControlByName(rf::local_player->settings, bind_name.c_str());
            if (ctrl == -1) {
                rf::console_printf("Cannot find control: %s", bind_name.c_str());
            }
            else {
                g_game_config.skip_cutscene_ctrl = ctrl;
                g_game_config.save();
                rf::console_printf("Skip Cutscene bind changed to: %s", bind_name.c_str());
            }
        }
    },
    "Changes bind used to skip cutscenes",
    "skip_cutscene_bind ctrl_name",
};

CodeInjection cutscene_shot_sync_fix{
    0x0045B43B,
    [](auto& regs) {
        auto& current_shot_idx = struct_field_ref<int>(rf::active_cutscene, 0x808);
        auto& current_shot_timer = struct_field_ref<rf::Timestamp>(rf::active_cutscene, 0x810);
        if (current_shot_idx > 1) {
            // decrease time for next shot using current shot timer value
            int shot_time_left_ms = current_shot_timer.time_until();
            if (shot_time_left_ms > 0 || shot_time_left_ms < -100)
                xlog::warn("invalid shot_time_left_ms %d", shot_time_left_ms);
            regs.eax += shot_time_left_ms;
        }
    },
};

void cutscene_apply_patches()
{
    // Support skipping cutscenes
    cutscene_do_frame_hook.install();
    skip_cutscene_bind_cmd.register_cmd();

    // Fix crash if camera cannot be restored to first-person mode after cutscene
    cutscene_stop_current_camera_set_first_person_hook.install();

    // Remove cutscene sync RF hackfix
    write_mem<float>(0x005897B4, 1000.0f);
    write_mem<float>(0x005897B8, 1.0f);
    static float zero = 0.0f;
    write_mem_ptr(0x0045B42A + 2, &zero);

    // Fix cutscene shot timer sync on high fps
    cutscene_shot_sync_fix.install();
}
