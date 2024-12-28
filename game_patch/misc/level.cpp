#include <xlog/xlog.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include "../os/console.h"
#include "../rf/multi.h"
#include "../rf/level.h"
#include "../rf/file/file.h"
#include "level.h"

CodeInjection level_read_data_check_restore_status_patch{
    0x00461195,
    [](auto& regs) {
        // check if sr_load_level_state is successful
        if (regs.eax != 0)
            return;
        // check if this is auto-load when changing level
        const char* save_filename = regs.edi;
        if (!std::strcmp(save_filename, "auto.svl"))
            return;
        // manual load failed
        xlog::error("Restoring game state failed");
        char* error_info = *reinterpret_cast<char**>(regs.esp + 0x2B0 + 0xC);
        std::strcpy(error_info, "Save file is corrupted");
        // return to level_read_data failure path
        regs.eip = 0x004608CC;
    },
};

CodeInjection level_load_items_crash_fix{
    0x0046519F,
    [](auto& regs) {
        void* item = regs.eax;
        if (item == nullptr) {
            regs.eip = 0x004651C6;
        }
    },
};

CallHook<void(rf::Vector3*, float, float, float, float, bool, int, int)> level_read_geometry_header_light_add_directional_hook{
    0x004619E1,
    [](rf::Vector3 *dir, float intensity, float r, float g, float b, bool is_dynamic, int casts_shadow, int dropoff_type) {
        if (rf::gr::lighting_enabled()) {
            level_read_geometry_header_light_add_directional_hook.call_target(dir, intensity, r, g, b, is_dynamic, casts_shadow, dropoff_type);
        }
    },
};

CallHook level_init_pre_console_output_hook{
    0x00435ABB,
    []() {
        rf::console::print("-- Level Initializing: {} --", rf::level_filename_to_load);
    },
};

CodeInjection level_load_init_inj{
    0x00460860,
    []() {
        DashLevelProps::instance() = {};
    },
};

CodeInjection level_load_chunk_inj{
    0x00460912,
    [](auto& regs) {
        int chunk_id = regs.eax;
        rf::File& file = addr_as_ref<rf::File>(regs.esp + 0x2B0 - 0x278);
        auto chunk_len = addr_as_ref<std::size_t>(regs.esp + 0x2B0 - 0x2A0);

        if (chunk_id == dash_level_props_chunk_id) {
            auto version = file.read<std::uint32_t>();
            if (version == 1) {
                DashLevelProps::instance().deserialize(file);
            } else {
                file.seek(chunk_len - 4, rf::File::seek_cur);
            }
            regs.eip = 0x004608EF;
        }
    },
};

void level_apply_patch()
{
    // Add checking if restoring game state from save file failed during level loading
    level_read_data_check_restore_status_patch.install();

    // Fix item_create null result handling in RFL loading (affects multiplayer only)
    level_load_items_crash_fix.install();

    // Fix dedicated server crash when loading level that uses directional light
    level_read_geometry_header_light_add_directional_hook.install();

    // Add level name to "-- Level Initializing --" message
    level_init_pre_console_output_hook.install();

    // Load custom rfl chunks
    level_load_init_inj.install();
    level_load_chunk_inj.install();
}
