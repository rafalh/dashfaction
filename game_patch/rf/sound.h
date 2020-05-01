#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"
#include <dsound.h>

namespace rf
{
    struct GameSound
    {
        char filename[32];
        float volume;
        float min_dist;
        float max_dist;
        float rolloff;
        int ds_snd_id;
        int field34_always0;
        int field_38;
        char b_loaded;
        char is_loaded;
        char is_looping;
        char field_3f_sndload_param;
    };
    static_assert(sizeof(GameSound) == 0x40);

    struct LevelSound
    {
        int sig;
        int game_snd_id;
        int type;
        int handle_hi_word;
        float vol_scale;
        float pan;
        Vector3 pos;
        float orig_volume;
        bool is_3d_sound;
    };
    static_assert(sizeof(LevelSound) == 0x2C);

    struct DsBuffer
    {
        char filename[256];
        char packfile[256];
        int packfile_offset;
        int name_checksum;
        int field_208_ds_load_buffer_param;
        IDirectSoundBuffer *ds_buffer;
        IDirectSound3DBuffer *ds_3d_buffer;
        WAVEFORMATEX *wave_format_orig;
        WAVEFORMATEX *wave_format_fixed;
        int field_21c;
        HMMIO hmmio;
        MMCKINFO data_chunk_info;
        MMCKINFO riff_chunk_info;
        int num_refs;
    };
    static_assert(sizeof(DsBuffer) == 0x250);

    struct DsChannel
    {
        IDirectSoundBuffer* sound_buffer;
        IDirectSound3DBuffer* sound_3d_buffer;
        int buf_id;
        int read_timer_id;
        int sig;
        float volume;
        int field_18_snd_ds_load_buffer_param;
        int field_1C;
        float fade_out_time_unk;
        Timer timer24;
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

    static auto& game_sounds = AddrAsRef<GameSound[2600]>(0x01CD3BA8);
    static auto& sound_enabled = AddrAsRef<bool>(0x017543D8);
#ifdef DASH_FACTION
    // In DF sound channels limit has been raised
    constexpr int num_sound_channels = 64;
    extern LevelSound level_sounds[num_sound_channels];
    extern DsChannel ds_channels[num_sound_channels];
#else
    constexpr int num_sound_channels = 30;
    static auto& level_sounds = AddrAsRef<LevelSound[num_sound_channels]>(0x01753C38);
    static auto& ds_channels = AddrAsRef<DsChannel[num_sound_channels]>(0x01AD7520);
#endif
    static auto& ds_buffers = AddrAsRef<DsBuffer[0x1000]>(0x01887388);

    static auto ClearLevelSound = AddrAsRef<int(LevelSound* lvl_snd)>(0x00505680);
    static auto DestroyAllPausedSounds = AddrAsRef<void()>(0x005059F0);
    static auto SetAllPlayingSoundsPaused = AddrAsRef<void(bool paused)>(0x00505C70);
    static auto SndStop = AddrAsRef<char(int sig)>(0x005442B0);
    static auto SndGetDuration = AddrAsRef<float(int game_snd_id)>(0x00544760);
    static auto SndDsCloseChannel = AddrAsRef<bool(int channel)>(0x00521930);
    static auto SndDsChannelIsPlaying = AddrAsRef<void*(int channel)>(0x005224D0);
    static auto SndDsIsChannelPaused = AddrAsRef<bool(int channel)>(0x00522500);
    static auto SndDsEstimateDuration = AddrAsRef<float(int snd_buf_id)>(0x00523170);
}
