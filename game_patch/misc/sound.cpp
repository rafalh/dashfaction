

#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/StaticBufferResizePatch.h>
#include <algorithm>
#include "../rf/sound.h"
#include "../rf/entity.h"
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

FunHook<int(const char*, float)> snd_music_play_cutscene_hook{
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

CodeInjection bink_set_sound_system_injection{
    0x00520BBC,
    [](auto& regs) {
        *reinterpret_cast<IDirectSound**>(regs.esp + 4) = rf::direct_sound;
    },
};

CodeInjection snd_ds_init_device_leave_injection{
    0x005215E0,
    []() {
        auto& ds_use_ds3d = addr_as_ref<bool>(0x01AED340);
        auto& ds_use_eax = addr_as_ref<bool>(0x01AD751C);
        xlog::info("DS3D: %d", ds_use_ds3d);
        xlog::info("EAX: %d", ds_use_eax);
    },
};

FunHook<int(int, const rf::Vector3&, float, const rf::Vector3&, int)> snd_play_3d_hook{
    0x005056A0,
    [](int handle, const rf::Vector3& pos, float vol_scale, const rf::Vector3& unk, int group) {
        if (!rf::ds3d_enabled) {
            return snd_play_3d_hook.call_target(handle, pos, vol_scale, unk, group);
        }
        if (!rf::sound_enabled || rf::snd_load_hint(handle) != 0) {
            return -1;
        }
        for (int i = 0; i < rf::num_sound_channels; ++i) {
            if (!rf::snd_pc_is_playing(rf::sound_instances[i].sig)) {
                rf::snd_pc_stop(rf::sound_instances[i].sig);
                rf::snd_clear_instance_slot(&rf::sound_instances[i]);
            }
        }
        for (int i = 0; i < rf::num_sound_channels; ++i) {
            auto& lvl_snd = rf::sound_instances[i];
            if (lvl_snd.handle < 0) {
                bool looping = rf::sounds[handle].is_looping;
                int sig = rf::snd_pc_play_3d(handle, pos, looping, 0);
                xlog::trace("snd_play_3d handle %d pos <%.2f %.2f %.2f> vol %.2f sig %d", handle, pos.x, pos.y, pos.z, vol_scale, sig);
                if (sig < 0) {
                    return -1;
                }

                float volume = vol_scale * rf::snd_group_volume[group];
                rf::snd_pc_set_volume(sig, volume);

                lvl_snd.sig = sig;
                lvl_snd.handle = handle;
                lvl_snd.group = group;
                lvl_snd.vol_scale = vol_scale;
                lvl_snd.pan = 0.0f;
                lvl_snd.is_3d_sound = true;
                lvl_snd.pos = pos;
                lvl_snd.orig_volume = volume;
                lvl_snd.use_count++;
                int instance_handle = i | (lvl_snd.use_count << 8);

                return instance_handle;
            }
        }
        xlog::warn("No free slot for 3D sound");
        return -1;
    },
};

FunHook<void(int, const rf::Vector3&, const rf::Vector3&, float)> snd_change_3d_hook{
    0x005058C0,
    [](int instance_handle, const rf::Vector3& pos, const rf::Vector3& vel, float volume) {
        if (!rf::ds3d_enabled) {
            snd_change_3d_hook.call_target(instance_handle, pos, vel, volume);
            return;
        }
        if (static_cast<uint8_t>(instance_handle) >= rf::num_sound_channels) {
            return;
        }
        auto& snd_inst = rf::sound_instances[static_cast<uint8_t>(instance_handle)];
        if (snd_inst.use_count != (instance_handle >> 8) || !snd_inst.is_3d_sound) {
            return;
        }
        if (volume != snd_inst.orig_volume) {
            snd_inst.orig_volume = volume;
            float scaled_volume = volume * rf::snd_group_volume[snd_inst.group];
            rf::snd_pc_set_volume(snd_inst.sig, scaled_volume);
        }

        snd_inst.pos = pos;
        auto chnl = rf::snd_ds_get_channel(snd_inst.sig);
        if (chnl >= 0) {
            rf::Sound* sound;
            rf::snd_pc_get_by_id(snd_inst.handle, &sound);
            rf::snd_ds3d_update_buffer(chnl, sound->min_dist, sound->max_dist, pos, vel);
        }
    },
};

void snd_update_sound_instances([[ maybe_unused ]] const rf::Vector3& camera_pos)
{
    // Update sound volume of all instances because we do not use Direct Sound 3D distance attenuation model
    for (auto& instance : rf::sound_instances) {
        if (instance.handle >= 0 && instance.is_3d_sound) {
            float volume = rf::snd_pc_calc_volume_3d(instance.handle, instance.pos, instance.orig_volume);
            float scaled_volume = volume * rf::snd_group_volume[instance.group];
            rf::snd_pc_set_volume(instance.sig, scaled_volume);
        }
    }
}

void snd_update_ambient_sounds(const rf::Vector3& camera_pos)
{
    // Play/stop ambiend sounds and update their volume
    for (auto& ambient_snd : rf::ambient_sounds) {
        if (ambient_snd.handle >= 0) {
            float distance = (camera_pos - ambient_snd.pos).len();
            auto& sound = rf::sounds[ambient_snd.handle];
            bool in_range = distance <= sound.max_dist;
            if (in_range) {
                if (ambient_snd.sig < 0) {
                    bool is_looping = rf::sounds[ambient_snd.handle].is_looping;
                    ambient_snd.sig = rf::snd_pc_play_3d(ambient_snd.handle, ambient_snd.pos, is_looping, 0);
                }
                float volume = rf::snd_pc_calc_volume_3d(ambient_snd.handle, ambient_snd.pos, ambient_snd.volume);
                float scaled_volume = volume * rf::snd_group_volume[0];
                rf::snd_pc_set_volume(ambient_snd.sig, scaled_volume);
            }
            else if (ambient_snd.sig >= 0 && !in_range) {
                rf::snd_pc_stop(ambient_snd.sig);
                ambient_snd.sig = -1;
            }
        }
    }
}

CodeInjection player_fpgun_play_action_anim_injection{
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

FunHook<void(const rf::Vector3&, const rf::Vector3&, const rf::Matrix3&)> snd_update_sounds_hook{
     0x00505EC0,
    [](const rf::Vector3& camera_pos, const rf::Vector3& camera_vel, const rf::Matrix3& camera_orient) {
        player_fpgun_move_sounds(camera_pos, camera_vel);

        // Use original implementation if DirectSound 3D is disabled
        if (!rf::ds3d_enabled) {
            snd_update_sounds_hook.call_target(camera_pos, camera_vel, camera_orient);
            return;
        }

        // Update DirectSound 3D listener parameters
        rf::snd_pc_change_listener(camera_pos, camera_vel, camera_orient);

        snd_update_sound_instances(camera_pos);
        snd_update_ambient_sounds(camera_pos);
    },
};

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

    // Update volume for non-looping sounds
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
    write_mem<i8>(0x005053F5 + 1, rf::num_sound_channels); // snd_init_sound_instances_array
    write_mem<i8>(0x005058D8 + 2, rf::num_sound_channels); // snd_change_3d
    write_mem<i8>(0x0050598A + 2, rf::num_sound_channels); // snd_change
    write_mem<i8>(0x00505A36 + 2, rf::num_sound_channels); // snd_stop_all_paused_sounds
    write_mem<i8>(0x00505A5A + 2, rf::num_sound_channels); // snd_stop
    write_mem<i8>(0x00505C31 + 2, rf::num_sound_channels); // snd_is_playing
    write_mem<i8>(0x00521145 + 1, rf::num_sound_channels); // snd_ds_channels_array_ctor
    write_mem<i8>(0x0052163D + 2, rf::num_sound_channels); // snd_ds_init_all_channels
    write_mem<i8>(0x0052191D + 2, rf::num_sound_channels); // snd_ds_close_all_channels
    write_mem<i8>(0x00522F4F + 2, rf::num_sound_channels); // snd_ds_get_channel
    write_mem<i8>(0x00522D1D + 2, rf::num_sound_channels); // snd_ds_close_all_channels2

    // Do not use DSPROPSETID_VoiceManager because it is not implemented by dsoal
    write_mem<i8>(0x00521597 + 4, 0);

    // Pass IDirectSound object to Bink Video engine
    bink_set_sound_system_injection.install();

    // do not set DS3DMODE_DISABLE on 3D buffers in snd_ds_play
    write_mem<u8>(0x005225D1, asm_opcodes::jmp_rel_short);

    // Use DirectSound 3D for 3D sounds
    snd_play_3d_hook.install();

    // Properly update DirectSound 3D sounds
    snd_change_3d_hook.install();

    // Log information about used sound API
    snd_ds_init_device_leave_injection.install();

    // Update fpgun 3D sounds positions
    player_fpgun_play_action_anim_injection.install();
    snd_update_sounds_hook.install();

    // Set rolloff factor to 0 for DirectSound 3D listener
    // It disables distance-based attenuation model from DirectSound 3D. It is necessary because Red Faction original
    // distance model is very different and must be emulated manually using SetVolume API
    write_mem<float>(0x00562EB1 + 1, 0.0f);
}

void register_sound_commands()
{
    level_sounds_cmd.register_cmd();
#ifdef DEBUG
    playing_sounds_cmd.register_cmd();
#endif
}
