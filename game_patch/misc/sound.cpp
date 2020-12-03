

#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/StaticBufferResizePatch.h>
#include <algorithm>
#include "../rf/sound.h"
#include "../main.h"
#include "../console/console.h"

static int g_cutscene_bg_sound_sig = -1;
std::vector<int> g_fpgun_sounds;

namespace rf
{
    rf::SoundInstance sound_instances[rf::num_sound_channels];
    rf::DsChannel ds_channels[rf::num_sound_channels];
}

void set_play_sound_events_volume_scale(float volume_scale)
{
    volume_scale = std::clamp(volume_scale, 0.0f, 1.0f);
    uintptr_t offsets[] = {
        // Play Sound event
        0x004BA4D8, 0x004BA515, 0x004BA71C, 0x004BA759, 0x004BA609, 0x004BA5F2, 0x004BA63F,
    };
    for (auto offset : offsets) {
        write_mem<float>(offset + 1, volume_scale);
    }
}

CallHook<void(int, rf::Vector3*, float*, float*, float)> snd_calculate_1d_from_3d_ambient_sound_hook{
    0x00505F93,
    [](int handle, rf::Vector3* sound_pos, float* pan_out, float* volume_out, float volume_in) {
        snd_calculate_1d_from_3d_ambient_sound_hook.call_target(handle, sound_pos, pan_out, volume_out, volume_in);
        *volume_out *= g_game_config.level_sound_volume;
    },
};

StaticBufferResizePatch<rf::SoundInstance> sound_instances_resize_patch{
    0x01753C38,
    30,
    rf::sound_instances,
    {
        {0x005053F9},
        {0x00505599},
        {0x005055CB},
        {0x0050571F},
        {0x005058E7},
        {0x00505A04},
        {0x00505A65},
        {0x00505A6C},
        {0x005060BB},
        {0x005058EE},
        {0x00505995},
        {0x0050599C},
        {0x00505C28},
        {0x00506197},
        {0x00505866},
        {0x00505F35},
        {0x005055C1, true},
        {0x005055DB, true},
        {0x00505A12, true},
        {0x005060EB, true},
        {0x00505893, true},
        {0x00505F68, true},
        {0x005061C1, true},
    },
};

StaticBufferResizePatch<rf::DsChannel> ds_channels_resize_patch{
    0x01AD7520,
    30,
    rf::ds_channels,
    {
        {0x00521149},
        {0x005211C3},
        {0x0052165D},
        {0x00522306},
        {0x005224DA},
        {0x0052250D},
        {0x005225A3},
        {0x005225B7},
        {0x005225F2},
        {0x00522608},
        {0x00522632},
        {0x0052265A},
        {0x00522734},
        {0x00522741},
        {0x00522763},
        {0x005227AD},
        {0x0052282F},
        {0x00522A61},
        {0x00522AAD},
        {0x00522B3A},
        {0x00522BDA},
        {0x00522C8E},
        {0x00522D4B},
        {0x00522D66},
        {0x00522D98},
        {0x00522DDF},
        {0x00522E1B},
        {0x00522F0B},
        {0x0052312B},
        {0x0054E8CC},
        {0x0054E956},
        {0x0054EAB5},
        {0x0054EB64},
        {0x0054EBB6},
        {0x0054EC93},
        {0x005211F5},
        {0x005225BD},
        {0x00562DC9},
        {0x0052264C},
        {0x00522781},
        {0x00522792},
        {0x00505E96},
        {0x00522688},
        {0x005226A0},
        {0x005227D8},
        {0x005227F0},
        {0x00522CFE},
        {0x00522F3A},
        {0x00522626},
        {0x0052275D},
        {0x00522D75},
        {0x0052262C},
        {0x0052271E},
        {0x00521945},
        {0x00522478},
        {0x0052251A},
        {0x005225E4},
        {0x005225EC},
        {0x00522724},
        {0x0052272E},
        {0x00522A57},
        {0x00522AA3},
        {0x00522AF4},
        {0x00522BE6},
        {0x00522BEE},
        {0x00522C04},
        {0x00522C57},
        {0x00522C80},
        {0x00522C9F},
        {0x00522CA7},
        {0x00522CB4},
        {0x0052194E},
        {0x00522F47, true},
        {0x005224A7, true},
        {0x00522B0B, true},
        {0x00522C29, true},
        {0x00522CCB, true},
    },
};

int try_to_free_ds_channel(float volume)
{
    int num_music = 0;
    int num_looping = 0;
    int num_long = 0;
    int chnl_id = -1;
    for (size_t i = 0; i < std::size(rf::ds_channels); ++i) {
        // Skip music and looping sounds
        if (rf::ds_channels[i].flags & rf::SCHF_MUSIC) {
            ++num_music;
            continue;
        }
        if (rf::ds_channels[i].flags & rf::SCHF_LOOPING) {
            ++num_looping;
            continue;
        }
        // Skip long sounds
        float duration = rf::snd_ds_estimate_duration(rf::ds_channels[i].buf_id);
        if (duration > 10.0f) {
            ++num_long;
            continue;
        }
        // Select channel with the lowest volume
        if (chnl_id < 0 || rf::ds_channels[chnl_id].volume < rf::ds_channels[i].volume) {
            chnl_id = static_cast<int>(i);
        }
    }
    // Free channel with the lowest volume and use it for new sound
    if (chnl_id > 0 && rf::ds_channels[chnl_id].volume <= volume) {
        xlog::debug("Freeing sound channel %d to make place for a new sound (volume %.4f duration %.1f)", chnl_id,
            rf::ds_channels[chnl_id].volume, rf::snd_ds_estimate_duration(rf::ds_channels[chnl_id].buf_id));
        rf::snd_ds_close_channel(chnl_id);
    }
    else {
        xlog::debug("Failed to allocate a sound channel: volume %.2f, num_music %d, num_looping %d, num_long %d", volume,
            num_music, num_looping, num_long);
    }
    return chnl_id;
}

CodeInjection snd_ds_play_3d_no_free_slots_fix{
    0x005222DF,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x14;
        auto volume_arg = addr_as_ref<float>(stack_frame + 0x18);
        if (regs.eax == -1) {
            regs.eax = try_to_free_ds_channel(volume_arg);
        }
    },
};

CodeInjection snd_ds_play_no_free_slots_fix{
    0x0052255C,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto volume_arg = addr_as_ref<float>(stack_frame + 0x8);
        if (regs.eax == -1) {
            regs.eax = try_to_free_ds_channel(volume_arg);
        }
    },
};

CodeInjection snd_play_no_free_slots_fix{
    0x005055E3,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto group_arg = addr_as_ref<int>(stack_frame + 0x8);
        auto vol_scale_arg = addr_as_ref<float>(stack_frame + 0x10);
        // Try to free a slot
        int num_looping = 0;
        int num_long = 0;
        int best_idx = -1;
        float best_vol_scale = 0.0f;
        for (size_t i = 0; i < std::size(rf::sound_instances); ++i) {
            auto& lvl_snd = rf::sound_instances[i];
            // Skip looping sounds
            if (rf::sounds[lvl_snd.handle].is_looping) {
                xlog::trace("Skipping sound %d because it is looping", i);
                ++num_looping;
                continue;
            }
            // Skip long sounds
            float duration = rf::snd_pc_get_duration(lvl_snd.handle);
            if (duration > 10.0f) {
                xlog::trace("Skipping sound %d because of duration: %f", i, duration);
                ++num_long;
                continue;
            }
            // Find sound with the lowest volume
            float current_vol_scale = rf::snd_group_volume[lvl_snd.group] * lvl_snd.vol_scale;
            if (best_idx == -1 || current_vol_scale < best_vol_scale) {
                best_idx = static_cast<int>(i);
                best_vol_scale = current_vol_scale;
            }
        }
        float new_sound_vol_scale = rf::snd_group_volume[group_arg] * vol_scale_arg;
        if (best_idx >= 0 && best_vol_scale <= new_sound_vol_scale) {
            // Free the selected slot and use it for a new sound
            auto& best_lvl_snd = rf::sound_instances[best_idx];
            xlog::debug("Freeing sound instance %d to make place for a new sound (volume %.4f duration %.1f)", best_idx,
                best_vol_scale, rf::snd_pc_get_duration(best_lvl_snd.handle));
            rf::snd_pc_stop(best_lvl_snd.sig);
            rf::snd_clear_instance_slot(&best_lvl_snd);
            regs.ebx = best_idx;
            regs.esi = reinterpret_cast<int32_t>(&best_lvl_snd);
            regs.eip = 0x005055EB;
        }
        else {
            xlog::debug("Failed to allocate a sound instance: volume %.2f num_looping %d num_long %d",
                new_sound_vol_scale, num_looping, num_long);
        }
    },
};

CallHook<void()> cutscene_play_music_hook{
    0x0045BB85,
    []() {
        g_cutscene_bg_sound_sig = -1;
        cutscene_play_music_hook.call_target();
    },
};

FunHook<int(const char*, float )> snd_music_play_cutscene_hook{
    0x00505D70,
    [](const char *filename, float volume) {
        g_cutscene_bg_sound_sig = snd_music_play_cutscene_hook.call_target(filename, volume);
        return g_cutscene_bg_sound_sig;
    },
};

void disable_sound_before_cutscene_skip()
{
    if (g_cutscene_bg_sound_sig != -1) {
        rf::snd_pc_stop(g_cutscene_bg_sound_sig);
        g_cutscene_bg_sound_sig = -1;
    }

    rf::snd_pause_all(true);
    rf::snd_destroy_all_paused();
    rf::sound_enabled = false;
}

void enable_sound_after_cutscene_skip()
{
    rf::sound_enabled = true;
}

void set_sound_enabled(bool enabled)
{
    rf::sound_enabled = enabled;
}

ConsoleCommand2 level_sounds_cmd{
    "levelsounds",
    [](std::optional<float> volume) {
        if (volume) {
            float vol_scale = std::clamp(volume.value(), 0.0f, 1.0f);
            //set_play_sound_events_volume_scale(vol_scale);

            g_game_config.level_sound_volume = vol_scale;
            g_game_config.save();
        }
        rf::console_printf("Level sound volume: %.1f", g_game_config.level_sound_volume.value());
    },
    "Sets level sounds volume scale",
    "levelsounds <volume>",
};

#ifdef DEBUG
ConsoleCommand2 playing_sounds_cmd{
    "d_playing_sounds",
    []() {
        for (int chnl_num = 0; chnl_num < rf::num_sound_channels; ++chnl_num) {
            auto& chnl = rf::ds_channels[chnl_num];
            if (chnl.sound_buffer) {
                DWORD status;
                chnl.sound_buffer->GetStatus(&status);
                auto& buf = rf::ds_buffers[chnl.buf_id];
                rf::console_printf("Channel %d: filename %s flags %x status %lx", chnl_num, buf.filename, chnl.flags, status);
            }
        }
    },
};
#endif

void register_sound_commands()
{
    level_sounds_cmd.register_cmd();
#ifdef DEBUG
    playing_sounds_cmd.register_cmd();
#endif
}

void apply_sound_patches()
{
    // Sound loop fix
    write_mem<u8>(0x00505D07 + 1, 0x00505D5B - (0x00505D07 + 2));

    // Cutscene skip support
    cutscene_play_music_hook.install();
    snd_music_play_cutscene_hook.install();

    // Level sounds
    set_play_sound_events_volume_scale(g_game_config.level_sound_volume);
    snd_calculate_1d_from_3d_ambient_sound_hook.install();

    AsmWriter(0x00505FE4).nop(2);
    // Fix ambient sound volume updating
    AsmWriter(0x00505FD7, 0x00505FFB)
        .mov(asm_regs::eax, *(asm_regs::esp + 0x50 + 4))
        .push(asm_regs::eax)
        .mov(asm_regs::eax, *(asm_regs::esi))
        .push(asm_regs::eax);

    // Delete sounds with lowest volume when there is no free slot for a new sound
    snd_ds_play_3d_no_free_slots_fix.install();
    snd_ds_play_no_free_slots_fix.install();
    snd_play_no_free_slots_fix.install();
    //write_mem<u8>(0x005055AB, asm_opcodes::jmp_rel_short); // never free level sounds, uncomment to test
    sound_instances_resize_patch.install();
    ds_channels_resize_patch.install();
    write_mem<i8>(0x005053F5 + 1, rf::num_sound_channels); // SndInitSoundInstancesArray
    write_mem<i8>(0x005058D8 + 2, rf::num_sound_channels); // snd_change_3d
    write_mem<i8>(0x0050598A + 2, rf::num_sound_channels); // SndChange
    write_mem<i8>(0x00505A36 + 2, rf::num_sound_channels); // SndStopAllPausedSounds
    write_mem<i8>(0x00505A5A + 2, rf::num_sound_channels); // SndStop
    write_mem<i8>(0x00505C31 + 2, rf::num_sound_channels); // snd_is_playing
    write_mem<i8>(0x00521145 + 1, rf::num_sound_channels); // SndDsChannelsArrayCtor
    write_mem<i8>(0x0052163D + 2, rf::num_sound_channels); // SndDsInitAllChannels
    write_mem<i8>(0x0052191D + 2, rf::num_sound_channels); // SndDsCloseAllChannels
    write_mem<i8>(0x00522F4F + 2, rf::num_sound_channels); // snd_ds_get_channel
    write_mem<i8>(0x00522D1D + 2, rf::num_sound_channels); // SndDsCloseAllChannels2
}
