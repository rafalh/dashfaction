#include "../rf/player.h"
#include "../rf/sound.h"
#include "../rf/vmesh.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <common/config/BuildConfig.h>
#include "../graphics/graphics.h"

static std::vector<int> g_fpgun_sounds;

static FunHook<void(rf::Player*)> player_fpgun_update_state_hook{
    0x004AA3A0,
    [](rf::Player* player) {
        player_fpgun_update_state_hook.call_target(player);
        if (player != rf::local_player) {
            rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
            if (entity) {
                float horz_speed_pow2 = entity->p_data.vel.x * entity->p_data.vel.x +
                                          entity->p_data.vel.z * entity->p_data.vel.z;
                int state = rf::WS_IDLE;
                if (rf::entity_weapon_is_on(entity->handle, entity->ai.current_primary_weapon))
                    state = rf::WS_LOOP_FIRE;
                else if (rf::entity_is_swimming(entity) || rf::entity_is_falling(entity))
                    state = rf::WS_IDLE;
                else if (horz_speed_pow2 > 0.2f)
                    state = rf::WS_RUN;
                if (!rf::player_fpgun_has_state(player, state))
                    rf::player_fpgun_set_state(player, state);
            }
        }
    },
};

static FunHook<void(rf::Player*)> player_fpgun_render_ir_hook{
    0x004AEEF0,
    [](rf::Player* player) {
        if (player->cam) {
            player_fpgun_render_ir_hook.call_target(player->cam->player);
        }
    },
};

static CodeInjection player_fpgun_play_action_anim_injection{
    0x004A947B,
    [](auto& regs) {
        if (regs.eax >= 0) {
            g_fpgun_sounds.push_back(regs.eax);
        }
    },
};

void player_fpgun_move_sounds(const rf::Vector3& camera_pos, const rf::Vector3& camera_vel)
{
    // Update position of fpgun sound
    auto it = g_fpgun_sounds.begin();
    while (it != g_fpgun_sounds.end()) {
        int sound_handle = *it;
        if (rf::snd_is_playing(sound_handle)) {
            rf::snd_change_3d(sound_handle, camera_pos, camera_vel, 1.0f);
            ++it;
        }
        else {
            it = g_fpgun_sounds.erase(it);
        }
    }
}

void player_fpgun_on_player_death(rf::Player* pp)
{
    // Reset fpgun animation when player dies
    rf::vmesh_stop_all_actions(pp->weapon_mesh_handle);
    rf::player_fpgun_clear_all_action_anim_sounds(pp);
}

CodeInjection railgun_scanner_start_render_to_texture{
    0x004ADD0A,
    [](auto& regs) {
        auto player = reinterpret_cast<rf::Player*>(regs.ebx);
        gr_begin_render_to_texture(player->ir_data.ir_bitmap_handle);
    },
};

CodeInjection player_fpgun_render_ir_begin_render_to_texture{
    0x004AF0BC,
    [](auto& regs) {
        auto player = reinterpret_cast<rf::Player*>(regs.esi);
        gr_begin_render_to_texture(player->ir_data.ir_bitmap_handle);
    },
};

CodeInjection after_game_render_to_dynamic_textures{
    0x00431890,
    []() {
        // Render to back-buffer from this point
        gr_end_render_to_texture();
    },
};

void player_fpgun_do_patch()
{
#if SPECTATE_MODE_SHOW_WEAPON
    AsmWriter(0x004AB1B8).nop(6); // player_fpgun_render
    AsmWriter(0x004AA23E).nop(6); // player_fpgun_setup_mesh
    AsmWriter(0x004AE0DF).nop(2); // player_fpgun_get_vmesh_handle

    AsmWriter(0x004A938F).nop(6);               // player_fpgun_play_action_anim
    write_mem<u8>(0x004A952C, asm_opcodes::jmp_rel_short); // player_fpgun_has_state
    AsmWriter(0x004AA56D).nop(6);               // player_fpgun_set_state
    AsmWriter(0x004AA6E7).nop(6);               // player_fpgun_process
    AsmWriter(0x004AE384).nop(6);               // player_fpgun_prepare_weapon
    write_mem<u8>(0x004ACE2C, asm_opcodes::jmp_rel_short); // player_fpgun_get_zoom

    player_fpgun_update_state_hook.install();

    // Render IR for player that is currently being shown by camera - needed for spectate mode
    player_fpgun_render_ir_hook.install();
#endif // SPECTATE_MODE_SHOW_WEAPON

    // Update fpgun 3D sounds positions
    player_fpgun_play_action_anim_injection.install();

    // Faster IR and Railgun scanner bitmap update
    railgun_scanner_start_render_to_texture.install();
    player_fpgun_render_ir_begin_render_to_texture.install();
    after_game_render_to_dynamic_textures.install();
}
