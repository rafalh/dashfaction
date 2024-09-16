#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <algorithm>
#include <xlog/xlog.h>
#include "../os/console.h"
#include "../rf/player/player.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/hud.h"
#include "../rf/cutscene.h"
#include "../rf/player/camera.h"
#include "../rf/os/frametime.h"
#include "../rf/os/timer.h"
#include "../main/main.h"
#include "../sound/sound.h"

static constexpr rf::ControlConfigAction default_skip_cutscene_ctrl = rf::CC_ACTION_MP_STATS;

static rf::String get_action_bind_name(int action)
{
    auto& config_item = rf::local_player->settings.controls.bindings[action];
    rf::String name;
    if (config_item.scan_codes[0] >= 0) {
        rf::control_config_get_key_name(&name, config_item.scan_codes[0]);
    }
    else if (config_item.mouse_btn_id >= 0) {
        rf::control_config_get_mouse_button_name(&name, config_item.mouse_btn_id);
    }
    else {
        name = "?";
    }
    return name;
}

static void render_skip_cutscene_hint_text(rf::ControlConfigAction action)
{
    if (rf::hud_disabled) {
        return;
    }
    auto bind_name = get_action_bind_name(action);
    auto& action_name = rf::local_player->settings.controls.bindings[action].name;
    rf::gr::set_color(255, 255, 255, 255);
    auto msg = std::format("Press {} ({}) to skip the cutscene", action_name, bind_name);
    auto x = rf::gr::screen_width() / 2;
    auto y = rf::gr::screen_height() - 30;
    rf::gr::string_aligned(rf::gr::ALIGN_CENTER, x, y, msg.c_str());
}

FunHook<void(bool)> cutscene_do_frame_hook{
    0x0045B5E0,
    [](bool dlg_open) {
        bool skip_cutscene = false;
        auto skip_cutscene_ctrl = g_game_config.skip_cutscene_ctrl.value() != -1
            ? static_cast<rf::ControlConfigAction>(g_game_config.skip_cutscene_ctrl.value())
            : default_skip_cutscene_ctrl;
        rf::control_config_check_pressed(&rf::local_player->settings.controls, skip_cutscene_ctrl, &skip_cutscene);

        if (!skip_cutscene) {
            cutscene_do_frame_hook.call_target(dlg_open);
            render_skip_cutscene_hint_text(skip_cutscene_ctrl);
        }
        else {
            xlog::info("Skipping cutscene...");
            disable_sound_before_cutscene_skip();

            while (rf::cutscene_is_playing()) {
                int shot_time_left_ms = rf::active_cutscene->next_stage_timestamp.time_until();

                if (rf::active_cutscene->current_script_index == rf::active_cutscene->num_cam_scripts - 1) {
                    // run last half second of last shot emulating 10 FPS so all events get properly processed before
                    // going back to normal gameplay
                    if (shot_time_left_ms > 500)
                        shot_time_left_ms -= 500;
                    else
                        shot_time_left_ms = std::min(shot_time_left_ms, 100);
                }
                rf::timer_add_delta_time(shot_time_left_ms);
                rf::frametime = shot_time_left_ms / 1000.0f;
                rf::timer_base -= static_cast<int64_t>(shot_time_left_ms) * rf::timer_freq / 1000;
                cutscene_do_frame_hook.call_target(dlg_open);
            }

            enable_sound_after_cutscene_skip();
            xlog::info("Finished skipping cutscene");
        }
    },
};

CallHook<bool(rf::Camera*)> cutscene_stop_current_camera_enter_first_person_hook{
    0x0045BDBD,
    [](rf::Camera* camera) {
        if (!cutscene_stop_current_camera_enter_first_person_hook.call_target(camera)) {
            rf::camera_enter_fixed(camera);
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
            int ctrl = rf::control_config_find_action_by_name(&rf::local_player->settings, bind_name.c_str());
            if (ctrl == -1) {
                rf::console::print("Cannot find control: {}", bind_name);
            }
            else {
                g_game_config.skip_cutscene_ctrl = ctrl;
                g_game_config.save();
                rf::console::print("Skip Cutscene bind changed to: {}", bind_name);
            }
        }
    },
    "Changes bind used to skip cutscenes",
    "skip_cutscene_bind ctrl_name",
};

CodeInjection cutscene_shot_sync_fix{
    0x0045B43B,
    [](auto& regs) {
        if (rf::active_cutscene->current_script_index > 1) {
            // decrease time for next shot using current shot timer value
            int shot_time_left_ms = rf::active_cutscene->next_stage_timestamp.time_until();
            if (shot_time_left_ms > 0 || shot_time_left_ms < -100)
                xlog::warn("invalid shot_time_left_ms {}", shot_time_left_ms);
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
    cutscene_stop_current_camera_enter_first_person_hook.install();

    // Remove cutscene sync RF hackfix
    write_mem<float>(0x005897B4, 1000.0f);
    write_mem<float>(0x005897B8, 1.0f);
    static float zero = 0.0f;
    write_mem_ptr(0x0045B42A + 2, &zero);

    // Fix cutscene shot timer sync on high fps
    cutscene_shot_sync_fix.install();
}
