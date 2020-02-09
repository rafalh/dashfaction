#pragma once

#include <patch_common/MemUtils.h>
#include "common.h"

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

    struct SoundChannel
    {
        void* sound_buffer;
        void* sound_3d_buffer;
        int snd_ds_id;
        int read_timer_id;
        int sig;
        float volume;
        int field_18_snd_ds_load_buffer_param;
        int field_1C;
        float fade_out_time_unk;
        Timer timer24;
        int flags;
    };
    static_assert(sizeof(SoundChannel) == 0x2C);

    enum SoundChannelFlags
    {
        SCHF_LOOPING = 0x1,
        SCHF_PAUSED = 0x2,
        SCHF_MUSIC = 0x4,
        SCHF_UNK8 = 0x8,
    };

    static auto& game_sounds = AddrAsRef<GameSound[2600]>(0x01CD3BA8);
    static auto& level_sounds = AddrAsRef<LevelSound[30]>(0x01753C38);
    static auto& snd_channels = AddrAsRef<SoundChannel[30]>(0x01AD7520);

    static auto ClearLevelSound = AddrAsRef<int(LevelSound* lvl_snd)>(0x00505680);
    static auto SndStop = AddrAsRef<char(int sig)>(0x005442B0);
    static auto SndGetDuration = AddrAsRef<float(int game_snd_id)>(0x00544760);
    static auto SndDsCloseChannel = AddrAsRef<bool(int channel)>(0x00521930);
    static auto SndDsChannelIsPlaying = AddrAsRef<void*(int channel)>(0x005224D0);
    static auto SndDsIsChannelPaused = AddrAsRef<bool(int channel)>(0x00522500);
    static auto SndDsEstimateDuration = AddrAsRef<float(int snd_buf_id)>(0x00523170);
}
