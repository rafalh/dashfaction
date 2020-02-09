

#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <algorithm>
#include "../rf/sound.h"
#include "../main.h"
#include "../console/console.h"

namespace rf
{

static auto& sound_enabled = AddrAsRef<bool>(0x017543D8);

} // namespace rf

static int g_cutscene_bg_sound_sig = -1;

void SetPlaySoundEventsVolumeScale(float volume_scale)
{
    volume_scale = std::clamp(volume_scale, 0.0f, 1.0f);
    uintptr_t offsets[] = {
        // Play Sound event
        0x004BA4D8, 0x004BA515, 0x004BA71C, 0x004BA759, 0x004BA609, 0x004BA5F2, 0x004BA63F,
    };
    for (auto offset : offsets) {
        WriteMem<float>(offset + 1, volume_scale);
    }
}

CallHook<void(int, rf::Vector3*, float*, float*, float)> SndConvertVolume3D_AmbientSound_hook{
    0x00505F93,
    [](int game_snd_id, rf::Vector3* sound_pos, float* pan_out, float* volume_out, float volume_in) {
        SndConvertVolume3D_AmbientSound_hook.CallTarget(game_snd_id, sound_pos, pan_out, volume_out, volume_in);
        *volume_out *= g_game_config.level_sound_volume;
    },
};

int TryToFreeDsChannel(float volume)
{
    int num_music = 0;
    int num_looping = 0;
    int num_long = 0;
    int chnl_id = -1;
    for (size_t i = 0; i < std::size(rf::snd_channels); ++i) {
        // Skip music and looping sounds
        if (rf::snd_channels[i].flags & rf::SCHF_MUSIC) {
            ++num_music;
            continue;
        }
        if (rf::snd_channels[i].flags & rf::SCHF_LOOPING) {
            ++num_looping;
            continue;
        }
        // Skip long sounds
        float duration = rf::SndDsEstimateDuration(rf::snd_channels[i].snd_ds_id);
        if (duration > 10.0f) {
            ++num_long;
            continue;
        }
        // Select channel with the lowest volume
        if (chnl_id < 0 || rf::snd_channels[chnl_id].volume < rf::snd_channels[i].volume) {
            chnl_id = static_cast<int>(i);
        }
    }
    // Free channel with the lowest volume and use it for new sound
    if (chnl_id > 0 && rf::snd_channels[chnl_id].volume <= volume) {
        INFO("Freeing sound channel %d to make place for a new sound (volume %.4f duration %.1f)", chnl_id,
            rf::snd_channels[chnl_id].volume, rf::SndDsEstimateDuration(rf::snd_channels[chnl_id].snd_ds_id));
        rf::SndDsCloseChannel(chnl_id);
    }
    else {
        WARN("Failed to allocate a sound channel: volume %.2f, num_music %d, num_looping %d, num_long %d", volume,
            num_music, num_looping, num_long);
    }
    return chnl_id;
}

CodeInjection snd_ds_play_3d_no_free_slots_fix{
    0x005222DF,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x14;
        auto volume_arg = AddrAsRef<float>(stack_frame + 0x18);
        if (regs.eax == -1) {
            regs.eax = TryToFreeDsChannel(volume_arg);
        }
    },
};

CodeInjection snd_ds_play_no_free_slots_fix{
    0x0052255C,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto volume_arg = AddrAsRef<float>(stack_frame + 0x8);
        if (regs.eax == -1) {
            regs.eax = TryToFreeDsChannel(volume_arg);
        }
    },
};

CodeInjection PlaySound_no_free_slots_fix{
    0x005055E3,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto type_arg = AddrAsRef<int>(stack_frame + 0x8);
        auto vol_scale_arg = AddrAsRef<float>(stack_frame + 0x10);
        auto& snd_vol_scale_per_type = AddrAsRef<float[4]>(0x01753C18);
        // Try to free a slot
        int num_looping = 0;
        int num_long = 0;
        int best_idx = -1;
        float best_vol_scale = 0.0f;
        for (size_t i = 0; i < std::size(rf::level_sounds); ++i) {
            auto& lvl_snd = rf::level_sounds[i];
            // Skip looping sounds
            if (rf::game_sounds[lvl_snd.game_snd_id].is_looping) {
                TRACE("Skipping sound %d because it is looping", i);
                ++num_looping;
                continue;
            }
            // Skip long sounds
            float duration = rf::SndGetDuration(lvl_snd.game_snd_id);
            if (duration > 10.0f) {
                TRACE("Skipping sound %d because of duration: %f", i, duration);
                ++num_long;
                continue;
            }
            // Find sound with the lowest volume
            float current_vol_scale = snd_vol_scale_per_type[lvl_snd.type] * lvl_snd.vol_scale;
            if (best_idx == -1 || current_vol_scale < best_vol_scale) {
                best_idx = static_cast<int>(i);
                best_vol_scale = current_vol_scale;
            }
        }
        float new_sound_vol_scale = snd_vol_scale_per_type[type_arg] * vol_scale_arg;
        if (best_idx >= 0 && best_vol_scale <= new_sound_vol_scale) {
            // Free the selected slot and use it for a new sound
            auto& best_lvl_snd = rf::level_sounds[best_idx];
            INFO("Freeing level sound %d to make place for a new sound (volume %.4f duration %.1f)", best_idx,
                best_vol_scale, rf::SndGetDuration(best_lvl_snd.game_snd_id));
            rf::SndStop(best_lvl_snd.sig);
            rf::ClearLevelSound(&best_lvl_snd);
            regs.ebx = best_idx;
            regs.esi = reinterpret_cast<int32_t>(&best_lvl_snd);
            regs.eip = 0x005055EB;
        }
        else {
            WARN("Failed to allocate a level sound: volume %.2f num_looping %d num_long %d ",
                new_sound_vol_scale, num_looping, num_long);
        }
    },
};

CallHook<int()> PlayHardcodedBackgroundMusicForCutscene_hook{
    0x0045BB85,
    []() {
        g_cutscene_bg_sound_sig = PlayHardcodedBackgroundMusicForCutscene_hook.CallTarget();
        return g_cutscene_bg_sound_sig;
    },
};

void DisableSoundBeforeCutsceneSkip()
{
    auto& snd_stop = AddrAsRef<char(int sig)>(0x005442B0);
    auto& destroy_all_paused_sounds = AddrAsRef<void()>(0x005059F0);
    auto& set_all_playing_sounds_paused = AddrAsRef<void(bool paused)>(0x00505C70);

    if (g_cutscene_bg_sound_sig != -1) {
        snd_stop(g_cutscene_bg_sound_sig);
        g_cutscene_bg_sound_sig = -1;
    }

    set_all_playing_sounds_paused(true);
    destroy_all_paused_sounds();
    rf::sound_enabled = false;
}

void EnableSoundAfterCutsceneSkip()
{
    rf::sound_enabled = true;
}

void SetSoundEnabled(bool enabled)
{
    rf::sound_enabled = enabled;
}

DcCommand2 level_sounds_cmd{
    "levelsounds",
    [](std::optional<float> volume) {
        if (volume) {
            float vol_scale = std::clamp(volume.value(), 0.0f, 1.0f);
            SetPlaySoundEventsVolumeScale(vol_scale);

            g_game_config.level_sound_volume = vol_scale;
            g_game_config.save();
        }
        rf::DcPrintf("Level sound volume: %.1f", g_game_config.level_sound_volume);
    },
    "Sets level sounds volume scale",
    "levelsounds <volume>",
};

void InitMiscSound()
{
    level_sounds_cmd.Register();
}

void ApplyMiscSoundPatches()
{
    // Sound loop fix
    WriteMem<u8>(0x00505D07 + 1, 0x00505D5B - (0x00505D07 + 2));

    // Cutscene skip support
    PlayHardcodedBackgroundMusicForCutscene_hook.Install();

    // Level sounds
    SetPlaySoundEventsVolumeScale(g_game_config.level_sound_volume);
    SndConvertVolume3D_AmbientSound_hook.Install();

    // Delete sounds with lowest volume when there is no free slot for a new sound
    snd_ds_play_3d_no_free_slots_fix.Install();
    snd_ds_play_no_free_slots_fix.Install();
    PlaySound_no_free_slots_fix.Install();
    //WriteMem<u8>(0x005055AB, asm_opcodes::jmp_rel_short); // never free level sounds, uncomment to test

}
