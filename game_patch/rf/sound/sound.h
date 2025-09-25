#pragma once

#include <patch_common/MemUtils.h>
#include "../math/vector.h"
#include "../math/matrix.h"
#include "../os/timestamp.h"

namespace rf
{
    struct Sound
    {
        char filename[32];
        float volume;
        float min_range;
        float max_range;
        float rolloff;
        int ds_snd_id;
        int field34_always0;
        int field_38;
        bool is_music_track;
        bool is_loaded;
        bool is_looping;
        char field_3f_sndload_param;
    };
    static_assert(sizeof(Sound) == 0x40);

    struct SoundInstance
    {
        int sig;
        int handle;
        int group;
        int use_count;
        float base_volume;
        float pan;
        Vector3 pos;
        float base_volume_3d;
        bool is_3d_sound;
    };
    static_assert(sizeof(SoundInstance) == 0x2C);

    enum SoundBackend
    {
        SOUND_BACKEND_NONE = 0,
        SOUND_BACKEND_DS = 1,
        SOUND_BACKEND_A3D = 2,
    };

    struct AmbientSound
    {
        int handle;
        int sig;
        Vector3 pos;
        float volume;
    };
    static_assert(sizeof(AmbientSound) == 0x18);

    constexpr int SOUND_GROUP_EFFECTS = 0;
    constexpr int SOUND_GROUP_MUSIC = 1;
    constexpr int SOUND_GROUP_VOICE_MESSAGES = 2;

    static auto& sound_backend = addr_as_ref<SoundBackend>(0x01CFC5D4);
    static auto& sounds = addr_as_ref<Sound[2600]>(0x01CD3BA8);
    static auto& sound_enabled = addr_as_ref<bool>(0x017543D8);
    static auto& ds3d_enabled = addr_as_ref<bool>(0x01AED340);
#ifdef DASH_FACTION
    // In DF sound channels limit has been raised
    constexpr int num_sound_channels = 64;
    extern SoundInstance sound_instances[num_sound_channels];
#else
    constexpr int num_sound_channels = 30;
    static auto& sound_instances = addr_as_ref<SoundInstance[num_sound_channels]>(0x01753C38);
#endif
    static auto& snd_group_volume = addr_as_ref<float[4]>(0x01753C18);
    static auto& ambient_sounds = addr_as_ref<AmbientSound[25]>(0x01754170);
    static auto& sound_listener_pos = addr_as_ref<Vector3>(0x01754160);
    static auto& sound_listener_rvec = addr_as_ref<Vector3>(0x01753C28);
    static auto& snd_music_sig = addr_as_ref<int>(0x017543D0);

    static auto& sound_pc_inited = addr_as_ref<bool>(0x01CFC5D0);

    static auto snd_load_hint = addr_as_ref<int(int handle)>(0x005054D0);
    static auto snd_play = addr_as_ref<int(int handle, int group, float pan, float volume)>(0x00505560);
    static auto snd_play_3d = addr_as_ref<int(int handle, const Vector3& pos, float vol_scale, const Vector3& unused, int group)>(0x005056A0);
    static auto snd_init_instance = addr_as_ref<int(SoundInstance* instance)>(0x00505680);
    static auto snd_stop_all_paused = addr_as_ref<void()>(0x005059F0);
    static auto snd_is_playing = addr_as_ref<int(int instance_handle)>(0x00505C00);
    static auto snd_pause = addr_as_ref<void(bool is_paused)>(0x00505C70);
    static auto snd_change_3d = addr_as_ref<void(int instance_handle, const Vector3& pos, const Vector3& vel, float vol_scale)>(0x005058C0);
    static auto snd_calculate_2d_from_3d_info = addr_as_ref<void(int handle, const Vector3& pos, float* pan, float* volume, float vol_multiplier)>(0x00505740);
    static auto snd_update_sounds = addr_as_ref<void(const Vector3& camera_pos, const Vector3& camera_vel, const Matrix3& camera_orient)>(0x00505EC0);

    static auto snd_pc_play = addr_as_ref<int(int handle, float vol_scale, float pan, float unused, bool is_final_volume)>(0x005439D0);
    static auto snd_pc_play_looping = addr_as_ref<int(int handle, float volume, float pan, float unused, bool skip_volume_scalling)>(0x00543A80);
    static auto snd_pc_play_3d = addr_as_ref<int(int handle, const Vector3& pos, bool looping, float unused)>(0x00544180);
    static auto snd_pc_stop = addr_as_ref<char(int sig)>(0x005442B0);
    static auto snd_pc_is_playing = addr_as_ref<bool(int sig)>(0x00544360);
    static auto snd_pc_set_volume = addr_as_ref<void(int sig, float volume)>(0x00544390);
    static auto snd_pc_set_pan = addr_as_ref<void(int sig, float pan)>(0x00544450);
    static auto snd_pc_get_by_id = addr_as_ref<int(int handle, Sound** sound)>(0x00544700);
    static auto snd_pc_get_duration = addr_as_ref<float(int handle)>(0x00544760);
    static auto snd_pc_change_listener = addr_as_ref<void(const Vector3& pos, const Vector3& vel, const Matrix3& orient)>(0x00543480);
    static auto snd_pc_calc_volume_3d = addr_as_ref<float(int handle, const Vector3& pos, float vol_scale)>(0x00543C20);
    static auto snd_pc_load_hint = addr_as_ref<int(int handle, bool skip_data_loading, bool is_bluebeard_btz)>(0x00543760);
    static auto snd_pc_is_ds3d_enabled = addr_as_ref<bool()>(0x00544750);
    static auto snd_pc_calculate_pan = addr_as_ref<float(const Vector3& pos)>(0x00543EA0);
    static auto snd_get_handle = addr_as_ref<int(const char*, float, float, float)>(0x005054B0);
}
