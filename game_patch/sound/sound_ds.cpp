#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/StaticBufferResizePatch.h>
#include <patch_common/AsmOpcodes.h>
#include <stb_vorbis.h>
#include <algorithm>
#include "../rf/sound/sound.h"
#include "../rf/sound/sound_ds.h"
#include "../rf/crt.h"
#include "../rf/file/file.h"
#include "../os/console.h"

namespace rf
{
    rf::DsChannel ds_channels[rf::num_sound_channels];
}

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

static int ds_get_free_channel_new(int sid, float volume, bool is_looping)
{
    float duration = rf::snd_ds_estimate_duration(sid);
    int max_channels = static_cast<int>(std::size(rf::ds_channels));
    if (!is_looping && duration < 10.0f) {
        float effects_volume = rf::snd_group_volume[rf::SOUND_GROUP_EFFECTS];
        float normalized_volume = effects_volume > 0.001f ? volume / effects_volume : 0.0f;
        if (normalized_volume < 0.1f) {
            xlog::trace("sound %d channel priority 1", sid);
            max_channels = max_channels / 4;
        }
        else if (normalized_volume < 0.2f) {
            xlog::trace("sound %d channel priority 2", sid);
            max_channels = max_channels * 2 / 4;
        }
        else if (normalized_volume < 0.5f) {
            xlog::trace("sound %d channel priority 3", sid);
            max_channels = max_channels * 3 / 4;
        }
        else {
            xlog::trace("sound %d channel priority 4", sid);
        }
    }
    for (int i = 0; i < max_channels; ++i) {
        auto& channel = rf::ds_channels[i];
        if (!channel.pdsb) {
            return i;
        }
        if (channel.flags & (rf::SCHF_MUSIC | rf::SCHF_LOOPING | rf::SCHF_PAUSED)) {
            continue;
        }
        DWORD status;
        bool is_playing = SUCCEEDED(channel.pdsb->GetStatus(&status)) && (status & DSBSTATUS_PLAYING) != 0;
        if (!is_playing) {
            rf::snd_ds_close_channel(i);
            return i;
        }
    }
    xlog::trace("No free channel for sound %d", sid);
    return -1;
}

static CodeInjection snd_ds_play_3d_get_free_channel_injection{
    0x005222DA,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x14;
        auto sid = addr_as_ref<int>(stack_frame + 0x4);
        auto volume = addr_as_ref<float>(stack_frame + 0x18);
        auto is_looping = addr_as_ref<bool>(stack_frame + 0x1C);
        regs.eax = ds_get_free_channel_new(sid, volume, is_looping);
        regs.eip = 0x005222DF;
    },
};

static CodeInjection snd_ds_play_get_free_channel_injection{
    0x00522557,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x10;
        auto sid = addr_as_ref<int>(stack_frame + 0x4);
        auto volume = addr_as_ref<float>(stack_frame + 0x8);
        auto is_looping = addr_as_ref<bool>(stack_frame + 0x10);
        regs.eax = ds_get_free_channel_new(sid, volume, is_looping);
        regs.eip = 0x0052255C;
    },
};

CodeInjection snd_ds_init_device_leave_injection{
    0x005215E0,
    []() {
        auto& ds_use_ds3d = addr_as_ref<bool>(0x01AED340);
        auto& ds_use_eax = addr_as_ref<bool>(0x01AD751C);
        xlog::info("DirectSound3D: %d", ds_use_ds3d);
        xlog::info("EAX sound: %d", ds_use_eax);
    },
};

static void ds_populate_new_sig(int channel)
{
    static auto sig_counter = 1;
    rf::ds_channels[channel].sig = 0x40000000 | (sig_counter << 8) | channel;
    sig_counter = (sig_counter + 1) % 0x10000;
}

static CodeInjection snd_ds_play_sig_generation_patch{
    0x00522683,
    [](auto& regs) {
        int channel = static_cast<int>(regs.esi) / sizeof(rf::DsChannel);
        ds_populate_new_sig(channel);
        regs.eip = 0x005226A0;
    },
};

static CodeInjection snd_ds_play_3d_sig_generation_patch{
    0x005223EF,
    [](auto& regs) {
        int channel = static_cast<rf::DsChannel*>(regs.esi) - rf::ds_channels;
        ds_populate_new_sig(channel);
        regs.eip = 0x00522409;
    },
};

static CodeInjection snd_ds_play_music_sig_generation_patch{
    0x005227D3,
    [](auto& regs) {
        int channel = static_cast<int>(regs.esi) / sizeof(rf::DsChannel);
        ds_populate_new_sig(channel);
        regs.eip = 0x005227F0;
    },
};

static FunHook<int(int)> snd_ds_get_channel_hook{
    0x00522F30,
    [](int sig) -> int {
        if (sig < 0) {
            return -1;
        }
        int index = static_cast<rf::ubyte>(sig);
        if (rf::ds_channels[index].sig != sig) {
            return -1;
        }
        return index;
    },
};

#ifdef DEBUG
ConsoleCommand2 playing_sounds_cmd{
    "d_playing_sounds",
    []() {
        for (unsigned chnl_num = 0; chnl_num < std::size(rf::ds_channels); ++chnl_num) {
            auto& chnl = rf::ds_channels[chnl_num];
            if (chnl.pdsb) {
                DWORD status;
                chnl.pdsb->GetStatus(&status);
                auto& buf = rf::ds_buffers[chnl.buf_id];
                rf::console::print("Channel {}: filename {} flags {:x} status {:x}", chnl_num, buf.filename, chnl.flags, status);
            }
        }
    },
};
#endif

struct MmioWrapper {
    HMMIO hmmio = nullptr;
    stb_vorbis* vorbis = nullptr;
};

FunHook<int(LPSTR, LONG, HMMIO*, WAVEFORMATEX**, WAVEFORMATEX**, LPMMCKINFO)> snd_mmio_open_hook{
    0x00563370,
    [](LPSTR filename, LONG offset, HMMIO* hmmio, WAVEFORMATEX **wfmt_orig, WAVEFORMATEX **wfmt_ds, LPMMCKINFO chunk_info) {
        char *sound_name = filename - 0x100;
        xlog::trace("Loading sound: %s", sound_name);
        if (!strcmp(rf::file_get_ext(sound_name), ".ogg")) {
            xlog::info("Loading Ogg Vorbis: %s", sound_name);
            int file_size = rf::File{}.size(sound_name);
            if (file_size <= 0) {
                xlog::error("Cannot get file size: %s", filename);
                return -1;
            }
            FILE *file = std::fopen(filename, "rb");
            if (!file) {
                xlog::error("Failed to open: %s", filename);
                return -1;
            }
            std::fseek(file, offset, SEEK_SET);
            int err = 0;
            stb_vorbis* vorbis = stb_vorbis_open_file_section(file, 1, &err, NULL, file_size);
            if (!vorbis) {
                xlog::error("Failed to load Ogg Vorbis: %d", err);
                return -1;
            }

            stb_vorbis_info info = stb_vorbis_get_info(vorbis);
            xlog::info("Ogg Vorbis info: channels %d, sample rate %d", info.channels, info.sample_rate);

            *wfmt_orig = reinterpret_cast<WAVEFORMATEX*>(rf::rf_malloc(sizeof(WAVEFORMATEX)));
            *wfmt_ds = reinterpret_cast<WAVEFORMATEX*>(rf::rf_malloc(sizeof(WAVEFORMATEX)));
            (*wfmt_orig)->wFormatTag = WAVE_FORMAT_PCM;
            (*wfmt_orig)->nChannels = info.channels;
            (*wfmt_orig)->wBitsPerSample = 16;
            (*wfmt_orig)->nSamplesPerSec = info.sample_rate;
            (*wfmt_orig)->nBlockAlign = (*wfmt_orig)->nChannels * (*wfmt_orig)->wBitsPerSample / 8;
            (*wfmt_orig)->nAvgBytesPerSec = (*wfmt_orig)->nSamplesPerSec * (*wfmt_orig)->nBlockAlign;
            (*wfmt_orig)->cbSize = 0;
            **wfmt_ds = **wfmt_orig;
            auto wrapper = new MmioWrapper;
            wrapper->vorbis = vorbis;
            *hmmio = reinterpret_cast<HMMIO>(wrapper);
            return 0;
        }
        auto wrapper = new MmioWrapper;
        *hmmio = reinterpret_cast<HMMIO>(wrapper);
        return snd_mmio_open_hook.call_target(filename, offset, &wrapper->hmmio, wfmt_orig, wfmt_ds, chunk_info);
    },
};

FunHook<int(HMMIO*, LPMMCKINFO, const MMCKINFO*)> snd_mmio_find_data_chunk_hook{
    0x005635D0,
    [](HMMIO* hmmio, LPMMCKINFO data_mmcki, const MMCKINFO* riff_mmcki) {
        auto wrapper = reinterpret_cast<MmioWrapper*>(*hmmio);
        if (wrapper->vorbis) {
            xlog::info("Ogg Vorbis seek start");
            stb_vorbis_seek_start(wrapper->vorbis);
            stb_vorbis_info info = stb_vorbis_get_info(wrapper->vorbis);
            float length = stb_vorbis_stream_length_in_seconds(wrapper->vorbis);
            data_mmcki->cksize = info.sample_rate * info.channels * sizeof(short) * length;
            return 0;
        } else {
            return snd_mmio_find_data_chunk_hook.call_target(&wrapper->hmmio, data_mmcki, riff_mmcki);
        }
    },
};

FunHook<int(HMMIO, unsigned, BYTE*, MMCKINFO*, int*)> snd_mmio_read_chunk_hook{
    0x00563620,
    [](HMMIO hmmio, unsigned buf_size, BYTE *buf, MMCKINFO *mmcki, int *bytes_read) {
        auto wrapper = reinterpret_cast<MmioWrapper*>(hmmio);
        if (wrapper->vorbis) {
            stb_vorbis_info info = stb_vorbis_get_info(wrapper->vorbis);
            int samples_read = stb_vorbis_get_samples_short_interleaved(wrapper->vorbis, info.channels, reinterpret_cast<short*>(buf), buf_size / 2);
            *bytes_read = samples_read * info.channels * sizeof(short);
            xlog::info("Ogg Vorbis: read %d samples", samples_read);
            return 0;
        }
        return snd_mmio_read_chunk_hook.call_target(wrapper->hmmio, buf_size, buf, mmcki, bytes_read);
    },
};

FunHook<void(HMMIO*)> snd_mmio_close_hook{
    0x005636E0,
    [](HMMIO *hmmio) {
        auto wrapper = reinterpret_cast<MmioWrapper*>(*hmmio);
        if (!wrapper) {
            return;
        }
        if (wrapper->vorbis) {
            xlog::info("Closing Ogg Vorbis stream");
            stb_vorbis_close(wrapper->vorbis);
        } else {
            snd_mmio_close_hook.call_target(&wrapper->hmmio);
        }
        delete wrapper;
        *hmmio = nullptr;
    },
};

void snd_ds_apply_patch()
{
    // Delete sounds with lowest volume when there is no free slot for a new sound
    snd_ds_play_3d_get_free_channel_injection.install();
    snd_ds_play_get_free_channel_injection.install();

    // Change number of sound channels
    ds_channels_resize_patch.install();
    auto max_ds_channels = static_cast<i8>(std::size(rf::ds_channels));
    write_mem<i8>(0x00521145 + 1, max_ds_channels); // snd_ds_channels_array_ctor
    write_mem<i8>(0x0052163D + 2, max_ds_channels); // snd_ds_init_all_channels
    write_mem<i8>(0x0052191D + 2, max_ds_channels); // snd_ds_close_all_channels
    write_mem<i8>(0x00522F4F + 2, max_ds_channels); // snd_ds_get_channel
    write_mem<i8>(0x00522D1D + 2, max_ds_channels); // snd_ds_close_all_channels2

    // Do not use DSPROPSETID_VoiceManager because it is not implemented by dsoal
    write_mem<i8>(0x00521597 + 4, 0);

    // Log information about used sound API
    snd_ds_init_device_leave_injection.install();

    // Set rolloff factor to 0 for DirectSound 3D listener
    // It disables distance-based attenuation model from DirectSound 3D. It is necessary because Red Faction original
    // distance model is very different and must be emulated manually using SetVolume API
    write_mem<float>(0x00562EB1 + 1, 0.0f);

    // Optimize performance of mapping a sig to DS channel number
    snd_ds_play_sig_generation_patch.install();
    snd_ds_play_3d_sig_generation_patch.install();
    snd_ds_play_music_sig_generation_patch.install();
    snd_ds_get_channel_hook.install();

    // Add support for more formats
    snd_mmio_open_hook.install();
    snd_mmio_find_data_chunk_hook.install();
    snd_mmio_read_chunk_hook.install();
    snd_mmio_close_hook.install();

    // Commands
#ifdef DEBUG
    playing_sounds_cmd.register_cmd();
#endif
}
