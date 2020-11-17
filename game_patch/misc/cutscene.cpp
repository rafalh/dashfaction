#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <algorithm>
#include "../console/console.h"
#include "../rf/player.h"
#include "../rf/graphics.h"
#include "../rf/hud.h"
#include "../main.h"
#include "sound.h"

static constexpr rf::ControlAction default_skip_cutscene_ctrl = rf::CA_MP_STATS;

rf::String GetGameCtrlBindName(int game_ctrl)
{
    auto GetKeyName = AddrAsRef<int(rf::String *out, int key)>(0x0043D930);
    auto GetMouseButtonName = AddrAsRef<int(rf::String *out, int mouse_btn)>(0x0043D970);
    auto ctrl_config = rf::local_player->settings.controls.keys[game_ctrl];
    rf::String name;
    if (ctrl_config.scan_codes[0] >= 0) {
        GetKeyName(&name, ctrl_config.scan_codes[0]);
    }
    else if (ctrl_config.mouse_btn_id >= 0) {
        GetMouseButtonName(&name, ctrl_config.mouse_btn_id);
    }
    else {
        return rf::String::Format("?");
    }
    return name;
}

void RenderSkipCutsceneHintText(rf::ControlAction ctrl)
{
    if (rf::is_hud_hidden) {
        return;
    }
    auto bind_name = GetGameCtrlBindName(ctrl);
    auto& ctrl_name = rf::local_player->settings.controls.keys[ctrl].name;
    rf::GrSetColorRgba(255, 255, 255, 255);
    auto msg = rf::String::Format("Press %s (%s) to skip the cutscene", ctrl_name.CStr(), bind_name.CStr());
    auto x = rf::GrScreenWidth() / 2;
    auto y = rf::GrScreenHeight() - 30;
    rf::GrStringAligned(rf::GR_ALIGN_CENTER, x, y, msg.CStr(), -1, rf::gr_string_mode);
}

FunHook<void(bool)> CutsceneDoFrame_hook{
    0x0045B5E0,
    [](bool dlg_open) {
        bool skip_cutscene = false;
        auto skip_cutscene_ctrl = g_game_config.skip_cutscene_ctrl.value() != -1
            ? static_cast<rf::ControlAction>(g_game_config.skip_cutscene_ctrl.value())
            : default_skip_cutscene_ctrl;
        rf::ControlConfigCheckPressed(&rf::local_player->settings.controls, skip_cutscene_ctrl, &skip_cutscene);

        if (!skip_cutscene) {
            CutsceneDoFrame_hook.CallTarget(dlg_open);
            RenderSkipCutsceneHintText(skip_cutscene_ctrl);
        }
        else {
            auto& timer_add_delta_time = AddrAsRef<int(int delta_ms)>(0x004FA2D0);

            auto& timer_base = AddrAsRef<int64_t>(0x01751BF8);
            auto& timer_freq = AddrAsRef<int32_t>(0x01751C04);
            auto& frame_time = AddrAsRef<float>(0x005A4014);
            auto& num_shots = StructFieldRef<int>(rf::active_cutscene, 4);
            auto& current_shot_idx = StructFieldRef<int>(rf::active_cutscene, 0x808);
            auto& current_shot_timer = StructFieldRef<rf::Timestamp>(rf::active_cutscene, 0x810);

            DisableSoundBeforeCutsceneSkip();

            while (rf::CutsceneIsActive()) {
                int shot_time_left_ms = current_shot_timer.TimeUntil();

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
                CutsceneDoFrame_hook.CallTarget(dlg_open);
            }

            EnableSoundAfterCutsceneSkip();
        }
    },
};

CallHook<bool(rf::Camera*)> CutsceneStopCurrent_CameraSetFirstPerson_hook{
    0x0045BDBD,
    [](rf::Camera* camera) {
        if (!CutsceneStopCurrent_CameraSetFirstPerson_hook.CallTarget(camera)) {
            rf::CameraSetFixed(camera);
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
            auto ConfigFindControlByName = AddrAsRef<int(rf::PlayerSettings&, const char*)>(0x0043D9F0);
            int ctrl = ConfigFindControlByName(rf::local_player->settings, bind_name.c_str());
            if (ctrl == -1) {
                rf::ConsolePrintf("Cannot find control: %s", bind_name.c_str());
            }
            else {
                g_game_config.skip_cutscene_ctrl = ctrl;
                g_game_config.save();
                rf::ConsolePrintf("Skip Cutscene bind changed to: %s", bind_name.c_str());
            }
        }
    },
    "Changes bind used to skip cutscenes",
    "skip_cutscene_bind ctrl_name",
};

void ApplyCutscenePatches()
{
    // Support skipping cutscenes
    CutsceneDoFrame_hook.Install();
    skip_cutscene_bind_cmd.Register();

    // Fix crash if camera cannot be restored to first-person mode after cutscene
    CutsceneStopCurrent_CameraSetFirstPerson_hook.Install();
}
