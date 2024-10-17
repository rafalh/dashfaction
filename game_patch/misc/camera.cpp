#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include "../main/main.h"
#include "../rf/multi.h"
#include "../os/console.h"
#include "../misc/misc.h"
#include "../multi/multi.h"
#include "../rf/player/player.h"
#include "../rf/os/frametime.h"

constexpr auto screen_shake_fps = 150.0f;

static float g_camera_shake_factor = 0.6f;

bool server_side_restrict_disable_ss = false;

FunHook<void(rf::Camera*)> camera_update_shake_hook{
    0x0040DB70,
    [](rf::Camera *camera) {
        float frame_time = rf::frametime;
        if (frame_time > 0.0001f) { // < 1000FPS
            // Fix screen shake caused by some weapons (eg. Assault Rifle)
            g_camera_shake_factor = std::pow(0.6f, frame_time / (1 / screen_shake_fps));
        }

        camera_update_shake_hook.call_target(camera);
    },
};

// called whenever screenshake is used, weapons, explosions, fall damage, events, etc.
FunHook<void(rf::Camera*, float, float)> camera_shake_hook{
    0x0040E0B0,
    [](rf::Camera* cp, float amplitude, float time_seconds) {

        if (g_game_config.try_disable_screenshake && !server_side_restrict_disable_ss) {
            return;
        }

        camera_shake_hook.call_target(cp, amplitude, time_seconds);
    }
};

void evaluate_restrict_disable_ss()
{
    server_side_restrict_disable_ss =
        rf::is_multi && !rf::is_server && get_df_server_info() && !get_df_server_info()->allow_no_ss;

    if (server_side_restrict_disable_ss) {
        if (g_game_config.try_disable_screenshake) {
            rf::console::print("This server does not allow you to disable screenshake!");
        }
    }
}

ConsoleCommand2 disable_screenshake_cmd{
    "disable_screenshake",
    []() {
        g_game_config.try_disable_screenshake = !g_game_config.try_disable_screenshake;
        g_game_config.save();

        evaluate_restrict_disable_ss();

        rf::console::print("Screenshake is {}",
                           g_game_config.try_disable_screenshake
                               ? "disabled. In multiplayer, this will only apply if the server allows it."
                               : "enabled.");
    },
    "Disable screenshake. In multiplayer, this is only applied if the server allows it.",
};

void camera_do_patch()
{
    // Fix crash when executing camera2 command in main menu
    AsmWriter(0x0040DCFC).nop(5);

    // Fix screen shake caused by some weapons (eg. Assault Rifle)
    write_mem_ptr(0x0040DBCC + 2, &g_camera_shake_factor);

    // handle turning off screen shake
    disable_screenshake_cmd.register_cmd();
    camera_shake_hook.install();
}
