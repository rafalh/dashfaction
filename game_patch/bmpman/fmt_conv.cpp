
#include <common/config/BuildConfig.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <algorithm>
#include <stdexcept>
#include "../rf/common.h"
#include <common/utils/perf-utils.h>
#include "../bmpman/bmpman.h"
#include "../bmpman/fmt_conv_templates.h"

bool bm_convert_format(void* dst_bits_ptr, rf::BmFormat dst_fmt, const void* src_bits_ptr,
                           rf::BmFormat src_fmt, int width, int height, int dst_pitch, int src_pitch,
                           const uint8_t* palette)
{
#if DEBUG_PERF
    static auto& color_conv_perf = PerfAggregator::create("bm_convert_format");
    ScopedPerfMonitor mon{color_conv_perf};
#endif
    try {
        call_with_format(src_fmt, [=](auto s) {
            call_with_format(dst_fmt, [=](auto d) {
                SurfacePixelFormatConverter<decltype(s)::value, decltype(d)::value> conv{
                    src_bits_ptr, dst_bits_ptr,
                    static_cast<size_t>(src_pitch), static_cast<size_t>(dst_pitch),
                    static_cast<size_t>(width), static_cast<size_t>(height),
                    palette,
                };
                conv();
            });
        });
        return true;
    }
    catch (const std::exception& e) {
        xlog::error("Pixel format conversion failed (%d -> %d): %s", src_fmt, dst_fmt, e.what());
        return false;
    }
}

rf::Color bm_get_pixel(uint8_t* data, rf::BmFormat format, int stride_in_bytes, int x, int y)
{
    if (bm_is_compressed_format(format)) {
        constexpr int block_w = 4;
        constexpr int block_h = 4;
        auto bytes_per_block = bm_get_bytes_per_compressed_block(format);
        auto block = data + (y / block_h) * stride_in_bytes + x / block_w * bytes_per_block;
        return decode_block_compressed_pixel(block, format, x % block_w, y % block_h);
    }
    else {
        auto ptr = data + y * stride_in_bytes + x * bm_bytes_per_pixel(format);
        rf::Color result{0, 0, 0, 0};
        call_with_format(format, [&](auto s) {
            PixelsReader<decltype(s)::value> rdr{ptr};
            PixelColor<rf::BmFormat::BM_FORMAT_8888_ARGB> color = rdr.read();
            result.set(
                static_cast<uint8_t>(color.r.value),
                static_cast<uint8_t>(color.g.value),
                static_cast<uint8_t>(color.b.value),
                static_cast<uint8_t>(color.a.value)
            );
        });
        return result;
    }
}
