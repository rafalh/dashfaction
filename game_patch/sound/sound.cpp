#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/StaticBufferResizePatch.h>
#include <algorithm>
#include "../rf/sound/sound.h"
#include "../rf/sound/sound_ds.h"
#include "../rf/entity.h"
#include "../rf/os/frametime.h"
#include "../main/main.h"
#include "../os/console.h"

static int g_cutscene_bg_sound_sig = -1;
#ifdef DEBUG
int g_sound_test = 0;
#endif

namespace rf
{
    rf::SoundInstance sound_instances[rf::num_sound_channels];
}

void player_fpgun_move_sounds(const rf::Vector3& camera_pos, const rf::Vector3& camera_vel);

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

static int snd_get_free_instance()
{
    for (unsigned i = 0; i < std::size(rf::sound_instances); ++i) {
        auto& instance = rf::sound_instances[i];
        if (instance.handle < 0) {
            return i;
        }
        if (!rf::snd_pc_is_playing(instance.sig)) {
            rf::snd_pc_stop(instance.sig);
            rf::snd_init_instance(&instance);
            return i;
        }
    }
    return -1;
}

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

    rf::snd_pause(true);
    rf::snd_stop_all_paused();
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
            set_play_sound_events_volume_scale(vol_scale);

            g_game_config.level_sound_volume = vol_scale;
            g_game_config.save();
        }
        rf::console::print("Level sound volume: {:.1f}", g_game_config.level_sound_volume.value());
    },
    "Sets level sounds volume scale",
    "levelsounds <volume>",
};

FunHook<int(int, int, float, float)> snd_play_hook{
    0x00505560,
    [](int handle, int group, float pan, float volume) {
        xlog::trace("snd_play %d %d %.2f %.2f", handle, group, pan, volume);

        if (!rf::sound_enabled || handle < 0) {
            return -1;
        }
        if (rf::snd_load_hint(handle) != 0) {
            xlog::warn("Failed to load sound %d", handle);
            return -1;
        }

        float vol_scale = volume * rf::snd_group_volume[group];
        bool looping = rf::sounds[handle].is_looping;
        int instance_index = snd_get_free_instance();
        if (instance_index < 0) {
            return -1;
        }

        auto& instance = rf::sound_instances[instance_index];
        int sig;
        if (looping) {
            sig = rf::snd_pc_play_looping(handle, vol_scale, pan, 0.0f, false);
        }
        else {
            sig = rf::snd_pc_play(handle, vol_scale, pan, 0.0f, false);
        }
        xlog::trace("snd_play handle %d vol %.2f sig %d", handle, vol_scale, sig);
        if (sig < 0) {
            return -1;
        }

        instance.sig = sig;
        instance.handle = handle;
        instance.group = group;
        instance.base_volume = volume;
        instance.pan = pan;
        instance.is_3d_sound = false;
        instance.use_count++;

        int instance_handle = instance_index | (instance.use_count << 8);
        return instance_handle;
    },
};

int snd_pc_play_3d_new(int handle, const rf::Vector3& pos, float vol_scale, bool looping, const rf::Vector3& vel = rf::zero_vector)
{
    // Original function has some flaws:
    // * it does not have vol_scale and vel parameters
    // * it mutes sounds below volume=0.05 (snd_pc_play does not do that)
    // * sounds.tbl volume is used (snd_update_sounds ignores it for 3D sounds)
    if (!rf::sound_pc_inited || handle == -1) {
        return -1;
    }
    int sid = rf::snd_pc_load_hint(handle, false, false);
    if (sid == -1) {
        return -1;
    }
    float pan;
    float volume_2d;
    rf::snd_calculate_2d_from_3d_info(handle, pos, &pan, &volume_2d, vol_scale);
    if (rf::sound_backend == rf::SOUND_BACKEND_DS) {
        if (rf::snd_pc_is_ds3d_enabled()) {
            // use a calculated volume scale that takes distance into account because we do not use
            // Direct Sound 3D distance attenuation model
            return rf::snd_ds_play_3d(sid, pos, vel,
                rf::sounds[handle].min_range,
                rf::sounds[handle].max_range,
                volume_2d,
                looping,
                0.0f);
        }
        float pan = rf::snd_pc_calculate_pan(pos);
        if (looping)
            return rf::snd_pc_play_looping(handle, volume_2d, pan, 0.0, true);
        return rf::snd_pc_play(handle, volume_2d, pan, 0.0f, true);
    }
    return -1;
}


FunHook<int(int, const rf::Vector3&, float, const rf::Vector3&, int)> snd_play_3d_hook{
    0x005056A0,
    [](int handle, const rf::Vector3& pos, float volume, const rf::Vector3&, int group) {
        xlog::trace("snd_play_3d %d %.2f %d", handle, volume, group);

        if (!rf::sound_enabled || handle < 0) {
            return -1;
        }
        if (rf::snd_load_hint(handle) != 0) {
            xlog::warn("Failed to load sound %d", handle);
            return -1;
        }


        bool looping = rf::sounds[handle].is_looping;
        int instance_index = snd_get_free_instance();
        if (instance_index < 0) {
            return -1;
        }

        float base_volume_2d;
        float pan;
        rf::snd_calculate_2d_from_3d_info(handle, pos, &pan, &base_volume_2d, volume);
        // xlog::warn("snd_play_3d vol %.2f", vol_scale);
        auto& instance = rf::sound_instances[instance_index];
        int sig;
        if (rf::ds3d_enabled) {
            // Note that snd_pc_play_3d_new calls snd_calculate_2d_from_3d_info on its own
            // so use volume here instead of base_volume_2d
            float vol_scale = volume * rf::snd_group_volume[group];
            sig = snd_pc_play_3d_new(handle, pos, vol_scale, looping);
        }
        else {
            float vol_scale = base_volume_2d * rf::snd_group_volume[group];
            if (looping) {
                sig = rf::snd_pc_play_looping(handle, vol_scale, pan, 0.0f, true);
            }
            else {
                sig = rf::snd_pc_play(handle, vol_scale, pan, 0.0f, true);
            }
        }
        xlog::trace("snd_play_3d handle %d pos <%.2f %.2f %.2f> vol %.2f sig %d", handle, pos.x, pos.y, pos.z, volume, sig);
        if (sig < 0) {
            return -1;
        }

        instance.sig = sig;
        instance.handle = handle;
        instance.group = group;
        instance.base_volume = base_volume_2d;
        instance.pan = pan;
        instance.is_3d_sound = true;
        instance.pos = pos;
        instance.base_volume_3d = volume;
        instance.use_count++;

        int instance_handle = instance_index | (instance.use_count << 8);
        return instance_handle;
    },
};

FunHook<void(int, const rf::Vector3&, const rf::Vector3&, float)> snd_change_3d_hook{
    0x005058C0,
    [](int instance_handle, const rf::Vector3& pos, const rf::Vector3& vel, float volume) {
        if (!rf::ds3d_enabled) {
            snd_change_3d_hook.call_target(instance_handle, pos, vel, volume);
            return;
        }
        auto instance_index = static_cast<uint8_t>(instance_handle);
        if (instance_index >= std::size(rf::sound_instances)) {
            assert(false);
            return;
        }
        auto& instance = rf::sound_instances[instance_index];
        if (instance.use_count != (instance_handle >> 8) || !instance.is_3d_sound) {
            return;
        }

        instance.pos = pos;

        if (volume != instance.base_volume_3d) {
            instance.base_volume_3d = volume;
            rf::snd_calculate_2d_from_3d_info(instance.handle, instance.pos, &instance.pan, &instance.base_volume, instance.base_volume_3d);
            float vol_scale = instance.base_volume * rf::snd_group_volume[instance.group];
            rf::snd_pc_set_volume(instance.sig, vol_scale);
        }

        auto chnl = rf::snd_ds_get_channel(instance.sig);
        if (chnl >= 0) {
            rf::Sound* sound;
            rf::snd_pc_get_by_id(instance.handle, &sound);
            rf::snd_ds3d_update_buffer(chnl, sound->min_range, sound->max_range, pos, vel);
        }
    },
};

void snd_update_sound_instances([[ maybe_unused ]] const rf::Vector3& camera_pos)
{
    // Update sound volume of all instances because we do not use Direct Sound 3D distance attenuation model
    for (auto& instance : rf::sound_instances) {
        if (instance.handle >= 0 && instance.is_3d_sound) {
            // Note: we should use snd_pc_calc_volume_3d because snd_calculate_2d_from_3d_info ignores volume from sounds.tbl
            //       but that is how it works in RF and fixing it would change volume levels for many sounds in game...
            rf::snd_calculate_2d_from_3d_info(instance.handle, instance.pos, &instance.pan, &instance.base_volume, instance.base_volume_3d);
            float vol_scale = instance.base_volume * rf::snd_group_volume[instance.group];
            rf::snd_pc_set_volume(instance.sig, vol_scale);
            if (!rf::ds3d_enabled) {
                rf::snd_pc_set_pan(instance.sig, instance.pan);
            }
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
            bool in_range = distance <= sound.max_range;
            // Note: when ambient sound is muted its volume is set to 0 (events can mute/unmute ambient sounds)
            // When it's muted destroy it so it can be recreated when unmuted and start playing from the beggining
            if (in_range && ambient_snd.volume > 0.0f) {
                float vol_scale = ambient_snd.volume * rf::snd_group_volume[rf::SOUND_GROUP_EFFECTS] * g_game_config.level_sound_volume;
                if (ambient_snd.sig < 0) {
                    bool is_looping = rf::sounds[ambient_snd.handle].is_looping;
                    ambient_snd.sig = snd_pc_play_3d_new(ambient_snd.handle, ambient_snd.pos, vol_scale, is_looping);
                }
                // Update volume even for non-looping sounds unlike original code
                float volume = rf::snd_pc_calc_volume_3d(ambient_snd.handle, ambient_snd.pos, vol_scale);
                rf::snd_pc_set_volume(ambient_snd.sig, volume);
                if (!rf::ds3d_enabled) {
                    float pan = rf::snd_pc_calculate_pan(ambient_snd.pos);
                    rf::snd_pc_set_pan(ambient_snd.sig, pan);
                }
            }
            else if (ambient_snd.sig >= 0) {
                rf::snd_pc_stop(ambient_snd.sig);
                ambient_snd.sig = -1;
            }
        }
    }
}

#ifdef DEBUG

ConsoleCommand2 sound_stress_test_cmd{
    "sound_stress_test",
    [](int num) {
        g_sound_test = num;
    },
};

void sound_test_do_frame()
{
    if (g_sound_test && rf::local_player_entity) {
        static float sounds_to_add = 0.0f;
        sounds_to_add += rf::frametime * g_sound_test;
        while (sounds_to_add >= 1.0f) {
            auto pos = rf::local_player_entity->p_data.pos;
            pos.x += ((rand() % 200) - 100) / 50.0f;
            pos.y += ((rand() % 200) - 100) / 50.0f;
            pos.z += ((rand() % 200) - 100) / 50.0f;
            rf::snd_play_3d(0, pos, 1.0f, rf::Vector3{}, 0);
            sounds_to_add -= 1.0f;
        }
    }
}

#endif // DEBUG

FunHook<void(const rf::Vector3&, const rf::Vector3&, const rf::Matrix3&)> snd_update_sounds_hook{
     0x00505EC0,
    [](const rf::Vector3& camera_pos, const rf::Vector3& camera_vel, const rf::Matrix3& camera_orient) {
        player_fpgun_move_sounds(camera_pos, camera_vel);

#ifdef DEBUG
        sound_test_do_frame();
#endif

        rf::sound_listener_pos = camera_pos;
        rf::sound_listener_rvec = camera_orient.rvec;

        // Update DirectSound 3D listener parameters
        rf::snd_pc_change_listener(camera_pos, camera_vel, camera_orient);

        snd_update_sound_instances(camera_pos);
        snd_update_ambient_sounds(camera_pos);
    },
};

FunHook<bool(const char*)> snd_pc_file_exists_hook{
    0x00544680,
    [](const char *filename) {
        bool exists = snd_pc_file_exists_hook.call_target(filename);

        if (!exists) {
            // For some reason built-in maps often use spaces in place of sound filename (their way to disable a sound?)
            // To reduce spam ignore space-only filenames and avoid logging the same filename multiple times in a row
            bool has_only_spaces = std::string_view{filename}.find_first_not_of(" ") == std::string_view::npos;
            static std::string last_file_not_found;
            if (!has_only_spaces && filename != last_file_not_found) {
                xlog::warn("Sound file not found: %s", filename);
                last_file_not_found = filename;
            }
        }
        return exists;
    }
};

FunHook<int(const char*, float, float, float)> snd_get_handle_hook{
    0x005054B0,
    [](const char* filename, float min_range, float base_volume, float rolloff) {
        if (!rf::sound_enabled) {
            return -1;
        }
        return snd_get_handle_hook.call_target(filename, min_range, base_volume, rolloff);
    },
};

void snd_ds_apply_patch();

void apply_sound_patches()
{
    // Sound loop fix in snd_music_play
    write_mem<u8>(0x00505D07 + 1, 0x00505D5B - (0x00505D07 + 2));

    // Cutscene skip support
    cutscene_play_music_hook.install();
    snd_music_play_cutscene_hook.install();

    // Level sounds
    set_play_sound_events_volume_scale(g_game_config.level_sound_volume);

    // Fix ambient sound volume updating
    AsmWriter(0x00505FD7, 0x00505FFB)
        .mov(asm_regs::eax, *(asm_regs::esp + 0x50 + 4))
        .push(asm_regs::eax)
        .mov(asm_regs::eax, *(asm_regs::esi))
        .push(asm_regs::eax);

    // Change number of sound channels
    //write_mem<u8>(0x005055AB, asm_opcodes::jmp_rel_short); // never free sound instances, uncomment to test
    sound_instances_resize_patch.install();
    auto max_sound_instances = static_cast<i8>(std::size(rf::sound_instances));
    write_mem<i8>(0x005053F5 + 1, max_sound_instances); // snd_init_sound_instances_array
    write_mem<i8>(0x005058D8 + 2, max_sound_instances); // snd_change_3d
    write_mem<i8>(0x0050598A + 2, max_sound_instances); // snd_change
    write_mem<i8>(0x00505A36 + 2, max_sound_instances); // snd_stop_all_paused_sounds
    write_mem<i8>(0x00505A5A + 2, max_sound_instances); // snd_stop
    write_mem<i8>(0x00505C31 + 2, max_sound_instances); // snd_is_playing

    // Improve handling of instance slot allocation in snd_play
    snd_play_hook.install();

    // Use DirectSound 3D for 3D sounds
    snd_play_3d_hook.install();

    // Properly update DirectSound 3D sounds
    snd_change_3d_hook.install();
    snd_update_sounds_hook.install();

    // Apply patch for DirectSound specific code
    snd_ds_apply_patch();

    // eax_ks_property_set should not call Release, since it is a weak pointer.
    AsmWriter{0x00527EE0, 0x00527EE4}.nop();

    // Change default volume to 0.5
    write_mem<float>(0x00506192 + 1, 0.5f);

    // Log when sound file cannot be found
    snd_pc_file_exists_hook.install();

    // Do not update sounds array in dedicated server mode
    snd_get_handle_hook.install();
}

void register_sound_commands()
{
    level_sounds_cmd.register_cmd();
#ifdef DEBUG
    sound_stress_test_cmd.register_cmd();
#endif
}
