#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include "../main/main.h"
#include "../rf/gr/gr.h"
#include "../rf/sound/sound_ds.h"

CodeInjection play_bik_file_infinite_loop_fix{
    0x00520BEE,
    [](auto& regs) {
        if (regs.eax == 0) {
            // pop edi
            regs.edi = *static_cast<int*>(regs.esp);
            regs.esp += 4;
            regs.eip = 0x00520C6E;
        }
    },
};

FunHook<unsigned()> bink_init_device_info_hook{
    0x005210C0,
    []() {
        unsigned bink_flags = bink_init_device_info_hook.call_target();
        const int BINKSURFACE32 = 3;

        if (g_game_config.true_color_textures && g_game_config.res_bpp == 32) {
            static auto& bink_bm_pixel_fmt = addr_as_ref<uint32_t>(0x018871C0);
            bink_bm_pixel_fmt = rf::bm::FORMAT_888_RGB;
            bink_flags = BINKSURFACE32;
        }

        return bink_flags;
    },
};

CodeInjection bink_set_sound_system_injection{
    0x00520BBC,
    [](auto& regs) {
        *reinterpret_cast<IDirectSound**>(regs.esp + 4) = rf::direct_sound;
    },
};

CallHook<void(int)> play_bik_file_vram_leak_fix{
    0x00520C79,
    [](int hbm) {
        rf::gr::tcache_add_ref(hbm);
        rf::gr::tcache_remove_ref(hbm);
        play_bik_file_vram_leak_fix.call_target(hbm);
    },
};

void bink_apply_patch()
{
    // Fix possible infinite loop when starting Bink video
    play_bik_file_infinite_loop_fix.install();

    // Fix memory leak when trying to play non-existing Bink video
    write_mem<int>(0x00520B7E + 2, 0x00520C6E - (0x00520B7E + 6));

    // True Color textures
    if (g_game_config.res_bpp == 32 && g_game_config.true_color_textures) {
        // use 32-bit texture for Bink rendering
        bink_init_device_info_hook.install();
    }

    // Pass IDirectSound object to Bink Video engine
    bink_set_sound_system_injection.install();

    // Fix PlayBikFile texture leak
    play_bik_file_vram_leak_fix.install();

    // Use renderer agnostic code in Bink rendering
    if (g_game_config.renderer != GameConfig::Renderer::legacy) {
        AsmWriter{0x00520ADA}.jmp(0x00520B24);
    }
}
