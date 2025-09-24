#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include "../rf/player/player.h"
#include "../rf/os/frametime.h"

constexpr auto screen_shake_fps = 150.0f;

static float g_camera_shake_factor = 0.6f;

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

CallHook<void(rf::Camera*, float, float)> entity_fire_primary_weapon_camera_shake_hook{
    0x00426C79,
    [] (rf::Camera* const cp, const float amplitude, const float time_seconds) {
        const bool server_force_weapon_shake = rf::is_multi
            && !rf::is_server
            && get_af_server_info()
            && !get_af_server_info()->allow_no_ss;
        if (g_game_config.weapon_shake || server_force_weapon_shake) {
            entity_fire_primary_weapon_camera_shake_hook.call_target(cp, amplitude, time_seconds);
        }
    },
};

ConsoleCommand2 weapon_shake_cmd{
    "weapon_shake",
    [] {
        g_game_config.weapon_shake = !g_game_config.weapon_shake;
        g_game_config.save();
        rf::console::print(
            "Weapon shake is {}",
            g_game_config.weapon_shake
                ? "enabled"
                : "disabled [multiplayer servers may force weapon shake]"
        );
    },
    "Toggle camera shake from weapon fire [mutiplayer servers may force weapon shake]",
};

void camera_do_patch()
{
    // Fix crash when executing camera2 command in main menu
    AsmWriter(0x0040DCFC).nop(5);

    // Fix screen shake caused by some weapons (eg. Assault Rifle)
    write_mem_ptr(0x0040DBCC + 2, &g_camera_shake_factor);
}
