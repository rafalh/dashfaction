#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include "../rf/player.h"

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

void camera_do_patch()
{
    // Fix crash when executing camera2 command in main menu
    AsmWriter(0x0040DCFC).nop(5);

    // Fix screen shake caused by some weapons (eg. Assault Rifle)
    write_mem_ptr(0x0040DBCC + 2, &g_camera_shake_factor);
}
