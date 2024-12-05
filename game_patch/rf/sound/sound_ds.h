#pragma once

#include <windows.h>
#include <mmeapi.h>
#include <mmiscapi.h>
#include <dsound.h>
#include <patch_common/MemUtils.h>
#include "sound.h"

namespace rf
{
    struct DsBuffer
    {
        char filename[256];
        char packfile[256];
        int packfile_offset;
        int name_checksum;
        int field_208_ds_load_buffer_param;
        IDirectSoundBuffer* ds_buffer;
        IDirectSound3DBuffer* ds_3d_buffer;
        WAVEFORMATEX* wave_format_orig;
        WAVEFORMATEX* wave_format_fixed;
        int field_21c;
        HMMIO hmmio;
        MMCKINFO data_chunk_info;
        MMCKINFO riff_chunk_info;
        int num_refs;
    };
    static_assert(sizeof(DsBuffer) == 0x250);

    struct DsChannel
    {
        IDirectSoundBuffer* pdsb;
        IDirectSound3DBuffer* pds3db;
        int buf_id;
        int read_timer_id;
        int sig;
        float volume;
        int field_18_snd_ds_load_buffer_param;
        int field_1C;
        float fade_out_time_unk;
        Timestamp timer24;
        int flags;
    };
    static_assert(sizeof(DsChannel) == 0x2C);

    enum DsChannelFlags
    {
        SCHF_LOOPING = 0x1,
        SCHF_PAUSED = 0x2,
        SCHF_MUSIC = 0x4,
        SCHF_UNK8 = 0x8,
    };

#ifdef DASH_FACTION
    // In DF sound channels limit has been raised
    extern DsChannel ds_channels[num_sound_channels];
#else
    static auto& ds_channels = addr_as_ref<DsChannel[num_sound_channels]>(0x01AD7520);
#endif
    static auto& direct_sound = addr_as_ref<IDirectSound*>(0x01AD7A50);
    static auto& ds_buffers = addr_as_ref<DsBuffer[0x1000]>(0x01887388);

    static auto snd_ds_close_channel = addr_as_ref<bool(int channel)>(0x00521930);
    static auto snd_ds_channel_is_playing = addr_as_ref<bool(int channel)>(0x005224D0);
    static auto snd_ds_channel_is_paused = addr_as_ref<bool(int channel)>(0x00522500);
    static auto snd_ds_get_channel = addr_as_ref<int(int sig)>(0x00522F30);
    static auto snd_ds_estimate_duration = addr_as_ref<float(int sid)>(0x00523170);
    static auto snd_ds_play_3d = addr_as_ref<
        int(int sid, const Vector3& pos, const Vector3& vel, float min_dist, float max_dist, float volume,
            bool is_looping, float unused)>(0x005222B0);

    static auto snd_ds3d_update_buffer =
        addr_as_ref<int(int chnl, float min_dist, float max_dist, const Vector3& pos, const Vector3& vel)>(0x00562DB0);
}
