#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <ddraw.h>
#include <cstddef>
#include "../rf.h"
#include "gr_color.h"

constexpr uint32_t dds_magic = 0x20534444;

struct DDS_PIXELFORMAT {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwFourCC;
  DWORD dwRGBBitCount;
  DWORD dwRBitMask;
  DWORD dwGBitMask;
  DWORD dwBBitMask;
  DWORD dwABitMask;
};

typedef struct {
  DWORD           dwSize;
  DWORD           dwFlags;
  DWORD           dwHeight;
  DWORD           dwWidth;
  DWORD           dwPitchOrLinearSize;
  DWORD           dwDepth;
  DWORD           dwMipMapCount;
  DWORD           dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  DWORD           dwCaps;
  DWORD           dwCaps2;
  DWORD           dwCaps3;
  DWORD           dwCaps4;
  DWORD           dwReserved2;
} DDS_HEADER;

namespace rf
{

class File
{
    using SelfType = File;

    char m_internal_data[0x114];

public:
    enum SeekOrigin {
        seek_set = 0,
        seek_cur = 1,
        seek_end = 2,
    };

    File()
    {
        auto fun_ptr = reinterpret_cast<void(__thiscall*)(SelfType*)>(0x00523940);
        fun_ptr(this);
    }

    ~File()
    {
        auto fun_ptr = reinterpret_cast<void(__thiscall*)(SelfType*)>(0x00523960);
        fun_ptr(this);
    }

    int Open(const char* filename, int flags = 1, int unk = 9999999)
    {
        auto fun_ptr = reinterpret_cast<int(__thiscall*)(SelfType*, const char*, int, int)>(0x00524190);
        return fun_ptr(this, filename, flags, unk);
    }

    void Close()
    {
        auto fun_ptr = reinterpret_cast<void(__thiscall*)(SelfType*)>(0x005242A0);
        fun_ptr(this);
    }

    bool CheckVersion(int min_ver) const
    {
        auto fun_ptr = reinterpret_cast<bool(__thiscall*)(const SelfType*, int)>(0x00523990);
        return fun_ptr(this, min_ver);
    }

    bool HasReadFailed() const
    {
        auto fun_ptr = reinterpret_cast<bool(__thiscall*)(const SelfType*)>(0x00524530);
        return fun_ptr(this);
    }

    int Seek(int pos, SeekOrigin origin)
    {
        auto fun_ptr = reinterpret_cast<int(__thiscall*)(const SelfType*, int, SeekOrigin)>(0x00524400);
        return fun_ptr(this, pos, origin);
    }

    int Read(void *buf, int buf_len, int min_ver = 0, int unused = 0)
    {
        auto fun_ptr = reinterpret_cast<int(__thiscall*)(SelfType*, void*, int, int, int)>(0x0052CF60);
        return fun_ptr(this, buf, buf_len, min_ver, unused);
    }

    template<typename T>
    T Read(int min_ver = 0, T def_val = 0)
    {
        if (CheckVersion(min_ver)) {
            T val;
            Read(&val, sizeof(val));
            if (!HasReadFailed()) {
                return val;
            }
        }
        return def_val;
    }
};

struct GrTextureSection
{
    IDirect3DTexture8 *d3d_texture;
    int num_vram_bytes;
    float u_scale;
    float v_scale;
    int x;
    int y;
    int width;
    int height;
};

}

constexpr rf::BmBitmapType dds_bm_type = static_cast<rf::BmBitmapType>(0x10);
constexpr rf::BmPixelFormat custom_pixel_fmt = static_cast<rf::BmPixelFormat>(0x10);

bool g_force_texture_in_backbuffer_format = false;

int GetSurfacePitch(int w, D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            return w * 4;
        case D3DFMT_R8G8B8:
            return w * 3;
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_A4R4G4B4:
            return w * 2;
        case D3DFMT_DXT1:
            // 4x4 pixels per block, 64 bits per block
            return (w + 3) / 4 * 64 / 8;
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            // 4x4 pixels per block, 128 bits per block
            return (w + 3) / 4 * 128 / 8;
        default:
            return -1;
    }
}

int GetSurfaceNumRows(int h, D3DFORMAT d3d_fmt)
{
    switch (d3d_fmt) {
        case D3DFMT_DXT1:
        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            // 4x4 pixels per block
            return (h + 3) / 4;
        default:
            return h;
    }
}

using GrD3DSetTextureData_Type =
    int(int, const uint8_t*, const uint8_t*, int, int, rf::BmPixelFormat, rf::GrTextureSection*, int, int, IDirect3DTexture8*);
FunHook<GrD3DSetTextureData_Type> gr_d3d_set_texture_data_hook{
    0x0055BA10,
    [](int level, const uint8_t* src_bits_ptr, const uint8_t* palette, int bm_w, int bm_h,
        rf::BmPixelFormat pixel_fmt, rf::GrTextureSection* section, int tex_w, int tex_h, IDirect3DTexture8* texture) {

        D3DLOCKED_RECT locked_rect;
        HRESULT hr = texture->LockRect(level, &locked_rect, 0, 0);
        if (FAILED(hr)) {
            ERR("LockRect failed");
            return -1;
        }
        D3DSURFACE_DESC desc;
        texture->GetLevelDesc(level, &desc);

        bool success = true;
        if (static_cast<int>(pixel_fmt) >= 0x10) {
            TRACE("Writing texture in custom format level %d src %p bm %dx%d tex %dx%d section %d %d",
                level, src_bits_ptr, bm_w, bm_h, tex_w, tex_h, section->x, section->y);
            auto d3d_fmt = static_cast<D3DFORMAT>(pixel_fmt);
            auto src_pitch = GetSurfacePitch(bm_w, d3d_fmt);
            auto num_src_rows = GetSurfaceNumRows(bm_h, d3d_fmt);

            auto src_ptr = reinterpret_cast<const std::byte*>(src_bits_ptr);
            auto dst_ptr = reinterpret_cast<std::byte*>(locked_rect.pBits);

            src_ptr += src_pitch * GetSurfaceNumRows(section->y, d3d_fmt);
            src_ptr += GetSurfacePitch(section->x, d3d_fmt);

            auto copy_pitch = GetSurfacePitch(section->width, d3d_fmt);

            for (int y = 0; y < num_src_rows; ++y) {
                std::memcpy(dst_ptr, src_ptr, copy_pitch);
                dst_ptr += locked_rect.Pitch;
                src_ptr += src_pitch;
            }
            TRACE("Writing completed");
        }
        else {
            auto tex_pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
            success = ConvertBitmapFormat(reinterpret_cast<uint8_t*>(locked_rect.pBits), tex_pixel_fmt,
                                          src_bits_ptr, pixel_fmt, bm_w, bm_h, locked_rect.Pitch,
                                          GetPixelFormatSize(pixel_fmt) * bm_w, palette);
            if (!success)
                WARN("Color conversion failed (format %d -> %d)", pixel_fmt, tex_pixel_fmt);
        }

        texture->UnlockRect(level);

        if (success)
            return 0;

        return gr_d3d_set_texture_data_hook.CallTarget(level, src_bits_ptr, palette, bm_w, bm_h, pixel_fmt, section,
                                                       tex_w, tex_h, texture);
    },
};

FunHook<int(rf::BmPixelFormat, int, int, int, IDirect3DTexture8**)> gr_d3d_create_vram_texture_hook{
    0x0055B970,
    [](rf::BmPixelFormat pixel_fmt, int width, int height, int levels, IDirect3DTexture8** texture_out) {
        D3DFORMAT d3d_format;
        if (g_force_texture_in_backbuffer_format) {
            TRACE("Forcing texture in backbuffer format");
            d3d_format = rf::gr_d3d_pp.BackBufferFormat;
        }
        else if (static_cast<unsigned>(pixel_fmt) >= 0x10) {
            d3d_format = static_cast<D3DFORMAT>(pixel_fmt);
        }
        else {
            return gr_d3d_create_vram_texture_hook.CallTarget(pixel_fmt, width, height, levels, texture_out);
        }

        TRACE("Creating texture in format %x", d3d_format);
        auto hr = rf::gr_d3d_device->CreateTexture(width, height, levels, 0, d3d_format, D3DPOOL_MANAGED, texture_out);
        if (FAILED(hr)) {
            ERR("Failed to create texture %dx%d in format %x", width, height, d3d_format);
            return -1;
        }
        return 0;
    },
};

CodeInjection gr_d3d_create_vram_texture_with_mipmaps_pitch_fix{
    0x0055B820,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x5C;
        auto pixel_fmt = AddrAsRef<rf::BmPixelFormat>(stack_frame - 0x30);
        auto& src_bits_ptr = regs.ecx;
        auto& num_total_vram_bytes = regs.ebp;
        int w = regs.esi;
        int h = regs.edi;

        regs.eip = 0x0055B82E;

        auto d3d_fmt = GetD3DFormatFromPixelFormat(pixel_fmt);
        int total_surface_bytes = GetSurfacePitch(w, d3d_fmt) * GetSurfaceNumRows(h, d3d_fmt);
        src_bits_ptr += total_surface_bytes;
        num_total_vram_bytes += total_surface_bytes;
    },
};

void RunWithTextureInBackBufferFormatForced(std::function<void()> callback)
{
    g_force_texture_in_backbuffer_format = true;
    callback();
    g_force_texture_in_backbuffer_format = false;
}

D3DFORMAT GetD3DFormatFromDDSPixelFormat(DDS_PIXELFORMAT& ddspf)
{
    if (ddspf.dwFlags & DDPF_RGB) {
        switch (ddspf.dwRGBBitCount) {
            case 32:
                if (ddspf.dwABitMask)
                    return D3DFMT_A8R8G8B8;
                else
                    return D3DFMT_X8R8G8B8;
            case 24:
                return D3DFMT_R8G8B8;
            case 16:
                if (ddspf.dwABitMask == 0x8000)
                    return D3DFMT_A1R5G5B5;
                else if (ddspf.dwABitMask)
                    return D3DFMT_A4R4G4B4;
                else
                    return D3DFMT_R5G6B5;
        }
    }
    else if (ddspf.dwFlags & DDPF_FOURCC) {
        switch (ddspf.dwFourCC) {
            case MAKEFOURCC('D', 'X', 'T', '1'):
                return D3DFMT_DXT1;
            case MAKEFOURCC('D', 'X', 'T', '2'):
                return D3DFMT_DXT2;
            case MAKEFOURCC('D', 'X', 'T', '3'):
                return D3DFMT_DXT3;
            case MAKEFOURCC('D', 'X', 'T', '4'):
                return D3DFMT_DXT4;
            case MAKEFOURCC('D', 'X', 'T', '5'):
                return D3DFMT_DXT5;
        }
    }
    WARN("Unsupported DDS pixel format");
    return D3DFMT_UNKNOWN;
}

rf::BmBitmapType ReadDdsHeader(rf::File& file, int *width_out, int *height_out, rf::BmPixelFormat *pixel_fmt_out,
    int *num_levels_out)
{
    auto magic = file.Read<uint32_t>();
    if (magic != dds_magic) {
        WARN("Invalid magic number in DDS file: %X", magic);
        return rf::BM_INVALID;
    }
    DDS_HEADER hdr;
    file.Read(&hdr, sizeof(hdr));
    if (hdr.dwSize != sizeof(DDS_HEADER)) {
        WARN("Invalid header size in DDS file: %lX", hdr.dwSize);
        return rf::BM_INVALID;
    }
    D3DFORMAT d3d_fmt = GetD3DFormatFromDDSPixelFormat(hdr.ddspf);
    if (d3d_fmt == D3DFMT_UNKNOWN) {
        return rf::BM_INVALID;
    }
    auto hr = rf::gr_d3d->CheckDeviceFormat(rf::gr_adapter_idx, D3DDEVTYPE_HAL, rf::gr_d3d_pp.BackBufferFormat, 0,
        D3DRTYPE_TEXTURE, d3d_fmt);
    if (FAILED(hr)) {
        WARN("Unsupported by video card DDS pixel format: %X", d3d_fmt);
        return rf::BM_INVALID;
    }
    TRACE("Using DDS format %X", d3d_fmt);

    *width_out = hdr.dwWidth;
    *height_out = hdr.dwHeight;

    // Use D3DFORMAT as BmPixelFormats - there are no conflicts because D3DFORMAT constants starts from 0x20
    *pixel_fmt_out = static_cast<rf::BmPixelFormat>(d3d_fmt);

    *num_levels_out = 1;
    if (hdr.dwFlags & DDSD_MIPMAPCOUNT) {
        *num_levels_out = hdr.dwMipMapCount;
        TRACE("DDS mipmaps %lu (%lux%lu)", hdr.dwMipMapCount, hdr.dwWidth, hdr.dwHeight);
    }
    return dds_bm_type;
}

int LockDdsBitmap(rf::BmBitmapEntry& bm_entry)
{
    rf::File file;
    std::string filename_without_ext{GetFilenameWithoutExt(bm_entry.name)};
    auto dds_filename = filename_without_ext + ".dds";

    TRACE("Locking DDS: %s", dds_filename.c_str());
    if (file.Open(dds_filename.c_str()) != 0) {
        ERR("failed to open DDS file: %s", dds_filename.c_str());
        return -1;
    }

    auto magic = file.Read<uint32_t>();
    if (magic != dds_magic) {
        ERR("Invalid magic number in %s: %x", dds_filename.c_str(), magic);
        return -1;
    }
    DDS_HEADER hdr;
    file.Read(&hdr, sizeof(hdr));

    if (bm_entry.orig_width != static_cast<int>(hdr.dwWidth) || bm_entry.orig_height != static_cast<int>(hdr.dwHeight)) {
        ERR("Bitmap file has changed before being loaded!");
        return -1;
    }

    int w = bm_entry.orig_width;
    int h = bm_entry.orig_height;
    int num_skip_levels = std::min(bm_entry.resolution_level, 2);
    auto d3d_fmt = GetD3DFormatFromPixelFormat(bm_entry.pixel_format);
    int num_skip_bytes = 0;
    int num_total_bytes = 0;

    for (int i = 0; i < num_skip_levels + bm_entry.num_levels; ++i) {
        int num_surface_bytes = GetSurfacePitch(w, d3d_fmt) * GetSurfaceNumRows(h, d3d_fmt);
        if (i < num_skip_levels) {
            num_skip_bytes += num_surface_bytes;
        }
        else {
            num_total_bytes += num_surface_bytes;
        }
        w = std::max(w / 2, 1);
        h = std::max(h / 2, 1);
    }

    // Skip levels with most details (depending on graphics settings)
    file.Seek(num_skip_bytes, rf::File::seek_cur);

    // Load actual data
    bm_entry.locked_data = rf::Malloc(num_total_bytes);
    file.Read(bm_entry.locked_data, num_total_bytes);
    if (file.HasReadFailed()) {
        ERR("Unexpected EOF when reading %s", dds_filename.c_str());
        return -1;
    }
    return 0;
}

FunHook<rf::BmBitmapType(const char*, int*, int*, rf::BmPixelFormat*, int*, int*, int*, int*, int*, int*, int)>
bm_read_header_hook{
    0x0050FCB0,
    [](const char* filename, int* width_out, int* height_out, rf::BmPixelFormat *pixel_fmt_out, int *num_levels_out,
    int *num_levels_external_mips_out, int *num_frames_out, int *fps_out, int *total_bytes_m2v_out,
    int *vbm_ver_out, int a11) {

        *num_levels_external_mips_out = 1;
        *num_levels_out = 1;
        *fps_out = 0;
        *num_frames_out = 1;
        *total_bytes_m2v_out = -1;
        *vbm_ver_out = 1;

        rf::File dds_file;
        std::string filename_without_ext{GetFilenameWithoutExt(filename)};
        auto dds_filename = filename_without_ext + ".dds";
        if (dds_file.Open(dds_filename.c_str()) == 0) {
            TRACE("Loading %s", dds_filename.c_str());
            auto bm_type = ReadDdsHeader(dds_file, width_out, height_out, pixel_fmt_out, num_levels_out);
            if (bm_type != rf::BM_INVALID) {
                return bm_type;
            }
        }

        return bm_read_header_hook.CallTarget(filename, width_out, height_out, pixel_fmt_out, num_levels_out,
            num_levels_external_mips_out, num_frames_out, fps_out, total_bytes_m2v_out, vbm_ver_out, a11);
    },
};

FunHook<rf::BmPixelFormat(int, void**, void**)> bm_lock_hook{
    0x00510780,
    [](int bmh, void** pixels_out, void** palette_out) {
        auto& bm_entry = rf::bm_bitmaps[rf::BmHandleToIdxAnimAware(bmh)];
        if (bm_entry.bitmap_type == dds_bm_type) {
            LockDdsBitmap(bm_entry);
            *pixels_out = bm_entry.locked_data;
            *palette_out = bm_entry.locked_palette;
            return bm_entry.pixel_format;
        }
        else {
            return bm_lock_hook.CallTarget(bmh, pixels_out, palette_out);
        }
    },
};

FunHook <void(int, float, float, rf::Color*)> gr_d3d_get_bitmap_pixel_hook{
    0x0055CFA0,
    [](int bmh, float u, float v, rf::Color* out_color) {
        if (static_cast<unsigned>(rf::BmGetPixelFormat(bmh)) > 0x10) {
            // Reading pixels is not yet supported in custom texture formats
            // This is only used when shooting at a texture with alpha
            out_color->red = 255;
            out_color->green = 255;
            out_color->blue = 255;
            out_color->alpha = 255;
        }
        else {
            gr_d3d_get_bitmap_pixel_hook.CallTarget(bmh, u, v, out_color);
        }
    },
};

// FunHook<void(int, int, int)> gr_d3d_set_state_and_texture_hook{
//     0x00550850,
//     [](int state, int bm0, int bm1) {
//         auto bm0_filename = rf::BmGetFilename(bm0);
//         if (bm0_filename && strstr(bm0_filename, "grate")) {
//             // disable alpha-blending
//             state &= ~(0x1F << 15);
//         }
//         gr_d3d_set_state_and_texture_hook.CallTarget(state, bm0, bm1);
//     },
// };

void ApplyTexturePatches()
{
    gr_d3d_set_texture_data_hook.Install();
    gr_d3d_create_vram_texture_hook.Install();
    bm_read_header_hook.Install();
    bm_lock_hook.Install();
    gr_d3d_create_vram_texture_with_mipmaps_pitch_fix.Install();
    gr_d3d_get_bitmap_pixel_hook.Install();
    //gr_d3d_set_state_and_texture_hook.Install();
}
