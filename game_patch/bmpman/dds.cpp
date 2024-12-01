#include <dds.h>
#include <xlog/xlog.h>
#include <common/utils/string-utils.h>
#include "../graphics/gr.h"
#include "../rf/bmpman.h"
#include "../rf/file/file.h"
#include "../rf/crt.h"
#include "bmpman.h"

rf::bm::Format get_bm_format_from_dds_pixel_format(DDS_PIXELFORMAT& ddspf)
{
    if (ddspf.flags & DDS_RGB) {
        switch (ddspf.RGBBitCount) {
            case 32:
                if (ddspf.ABitMask)
                    return rf::bm::FORMAT_8888_ARGB;
                else
                    break;
            case 24:
                return rf::bm::FORMAT_888_RGB;
            case 16:
                if (ddspf.ABitMask == 0x8000)
                    return rf::bm::FORMAT_1555_ARGB;
                else if (ddspf.ABitMask)
                    return rf::bm::FORMAT_4444_ARGB;
                else
                    return rf::bm::FORMAT_565_RGB;
        }
    }
    else if (ddspf.flags & DDS_FOURCC) {
        switch (ddspf.fourCC) {
            case MAKEFOURCC('D', 'X', 'T', '1'):
                return rf::bm::FORMAT_DXT1;
            case MAKEFOURCC('D', 'X', 'T', '2'):
                return rf::bm::FORMAT_DXT2;
            case MAKEFOURCC('D', 'X', 'T', '3'):
                return rf::bm::FORMAT_DXT3;
            case MAKEFOURCC('D', 'X', 'T', '4'):
                return rf::bm::FORMAT_DXT4;
            case MAKEFOURCC('D', 'X', 'T', '5'):
                return rf::bm::FORMAT_DXT5;
        }
    }
    xlog::warn("Unsupported DDS pixel format");
    return rf::bm::FORMAT_NONE;
}

rf::bm::Type
read_dds_header(rf::File& file, int* width_out, int* height_out, rf::bm::Format* format_out, int* num_levels_out)
{
    auto magic = file.read<uint32_t>();
    if (magic != DDS_MAGIC) {
        xlog::warn("Invalid magic number in DDS file: {:x}", magic);
        return rf::bm::TYPE_NONE;
    }
    DDS_HEADER hdr;
    file.read(&hdr, sizeof(hdr));
    if (hdr.size != sizeof(DDS_HEADER)) {
        xlog::warn("Invalid header size in DDS file: {:x}", hdr.size);
        return rf::bm::TYPE_NONE;
    }
    auto format = get_bm_format_from_dds_pixel_format(hdr.ddspf);
    if (format == rf::bm::FORMAT_NONE) {
        return rf::bm::TYPE_NONE;
    }

    if (!gr_is_texture_format_supported(format)) {
        xlog::warn("Unsupported by video card DDS pixel format: {:x}", static_cast<int>(format));
        return rf::bm::TYPE_NONE;
    }

    xlog::trace("Using DDS format 0x{:x}", format);

    *width_out = hdr.width;
    *height_out = hdr.height;
    *format_out = format;

    *num_levels_out = 1;
    if (hdr.flags & DDS_HEADER_FLAGS_MIPMAP) {
        *num_levels_out = hdr.mipMapCount;
        xlog::trace("DDS mipmaps {} ({}x{})", hdr.mipMapCount, hdr.width, hdr.height);
    }
    return rf::bm::TYPE_DDS;
}

int lock_dds_bitmap(rf::bm::BitmapEntry& bm_entry)
{
    rf::File file;
    std::string filename_without_ext{get_filename_without_ext(bm_entry.name)};
    auto dds_filename = filename_without_ext + ".dds";

    xlog::trace("Locking DDS: {}", dds_filename);
    if (file.open(dds_filename.c_str()) != 0) {
        xlog::error("failed to open DDS file: {}", dds_filename);
        return -1;
    }

    auto magic = file.read<uint32_t>();
    if (magic != DDS_MAGIC) {
        xlog::error("Invalid magic number in {}: {:x}", dds_filename, magic);
        return -1;
    }
    DDS_HEADER hdr;
    file.read(&hdr, sizeof(hdr));

    if (bm_entry.orig_width != static_cast<int>(hdr.width) || bm_entry.orig_height != static_cast<int>(hdr.height)) {
        xlog::error("Bitmap file has changed before being loaded!");
        return -1;
    }

    int w = bm_entry.orig_width;
    int h = bm_entry.orig_height;
    int num_skip_levels = std::min(bm_entry.resolution_level, 2);
    int num_skip_bytes = 0;
    int num_total_bytes = 0;

    for (int i = 0; i < num_skip_levels + bm_entry.num_levels; ++i) {
        int num_surface_bytes = bm_calculate_total_bytes(w, h, bm_entry.format);
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
    file.seek(num_skip_bytes, rf::File::seek_cur);

    // Load actual data
    bm_entry.locked_data = rf::rf_malloc(num_total_bytes);
    file.read(bm_entry.locked_data, num_total_bytes);
    if (file.error()) {
        xlog::error("Unexpected EOF when reading {}", dds_filename);
        return -1;
    }
    return 0;
}
