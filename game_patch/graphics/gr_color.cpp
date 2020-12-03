#include <windows.h>
#include <d3d8.h>
#include "gr_color.h"
#include "../main.h"
#include "../rf/graphics.h"
#include "../rf/geometry.h"
#include "../utils/perf-utils.h"
#include <common/BuildConfig.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <algorithm>
#include <stdexcept>

#define TEXTURE_DITHERING 0

template<int SRC_BITS, int DST_BITS>
struct ColorChannelConverter;

template<int BITS>
struct ColorChannel
{
    static constexpr int bits = BITS;
    static constexpr int max = (1 << BITS) - 1;

    int value = 0;

    template<int DST_BITS>
    constexpr operator ColorChannel<DST_BITS>() const
    {
        return ColorChannelConverter<BITS, DST_BITS>{}(*this);
    }

    ColorChannel<BITS> operator+(ColorChannel<BITS> other) const
    {
        return ColorChannel<BITS>{value + other.value};
    }

    ColorChannel<BITS> operator-(ColorChannel<BITS> other) const
    {
        return ColorChannel<BITS>{value - other.value};
    }

    ColorChannel<BITS> operator*(int n) const
    {
        return ColorChannel<BITS>{value * n};
    }

    ColorChannel<BITS> operator/(int n) const
    {
        return ColorChannel<BITS>{value / n};
    }
};

template<int SRC_BITS, int DST_BITS>
struct ColorChannelConverter
{
    constexpr ColorChannel<DST_BITS> operator()(ColorChannel<SRC_BITS> src)
    {
        return {(src.value * ColorChannel<DST_BITS>::max + ColorChannel<SRC_BITS>::max / 2)
            / ColorChannel<SRC_BITS>::max};
    }
};

template<int DST_BITS>
struct ColorChannelConverter<0, DST_BITS>
{
    constexpr ColorChannel<DST_BITS> operator()(ColorChannel<0>)
    {
        return {ColorChannel<DST_BITS>::max};
    }
};

template<int BITS>
struct ColorChannelConverter<BITS, BITS>
{
    constexpr ColorChannel<BITS> operator()(ColorChannel<BITS> src)
    {
        return src;
    }
};

static_assert(static_cast<ColorChannel<8>>(ColorChannel<5>{31}).value == 255);
static_assert(static_cast<ColorChannel<8>>(ColorChannel<5>{0}).value == 0);
static_assert(static_cast<ColorChannel<5>>(ColorChannel<8>{255}).value == 31);
static_assert(static_cast<ColorChannel<5>>(ColorChannel<8>{0}).value == 0);
static_assert(static_cast<ColorChannel<8>>(ColorChannel<0>{0}).value == 255);
static_assert(static_cast<ColorChannel<0>>(ColorChannel<0>{0}).value == 0);

template<unsigned BITS>
struct ChannelTrait
{
    static constexpr unsigned bits = BITS;
};

template<unsigned BITS, unsigned OFFSET>
struct ChannelBitOffsetTrait
{
    static constexpr unsigned bits = BITS;
    static constexpr unsigned offset = OFFSET;
};

template<unsigned BITS, unsigned INDEX>
struct ChannelIndexTrait
{
    static constexpr unsigned bits = BITS;
    static constexpr unsigned index = INDEX;
};

template<rf::BmFormat PF>
struct PixelFormatTrait;

template<>
struct PixelFormatTrait<rf::BM_FORMAT_8888_ARGB>
{
    using Pixel = uint32_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<8, 24>;
    using Red   = ChannelBitOffsetTrait<8, 16>;
    using Green = ChannelBitOffsetTrait<8, 8>;
    using Blue  = ChannelBitOffsetTrait<8, 0>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_888_RGB>
{
    using Pixel = uint8_t[3];
    using PaletteEntry = void;

    using Alpha = ChannelIndexTrait<0, 0>;
    using Red   = ChannelIndexTrait<8, 2>;
    using Green = ChannelIndexTrait<8, 1>;
    using Blue  = ChannelIndexTrait<8, 0>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_1555_ARGB>
{
    using Pixel = uint16_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<1, 15>;
    using Red   = ChannelBitOffsetTrait<5, 10>;
    using Green = ChannelBitOffsetTrait<5, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_565_RGB>
{
    using Pixel = uint16_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<0, 0>;
    using Red   = ChannelBitOffsetTrait<5, 11>;
    using Green = ChannelBitOffsetTrait<6, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_4444_ARGB>
{
    using Pixel = uint16_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<4, 12>;
    using Red   = ChannelBitOffsetTrait<4, 8>;
    using Green = ChannelBitOffsetTrait<4, 4>;
    using Blue  = ChannelBitOffsetTrait<4, 0>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_8_ALPHA>
{
    using Pixel = uint8_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<8, 0>;
    using Red   = ChannelBitOffsetTrait<0, 0>;
    using Green = ChannelBitOffsetTrait<0, 0>;
    using Blue  = ChannelBitOffsetTrait<0, 0>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_8_PALETTED>
{
    using Pixel = uint8_t;
    using PaletteEntry = uint8_t[3];

    using Alpha = ChannelIndexTrait<0, 0>;
    using Red = ChannelIndexTrait<8, 0>;
    using Green = ChannelIndexTrait<8, 1>;
    using Blue = ChannelIndexTrait<8, 2>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_888_BGR>
{
    using Pixel = uint8_t[3];
    using PaletteEntry = void;

    using Alpha = ChannelIndexTrait<0, 0>;
    using Red   = ChannelIndexTrait<8, 0>;
    using Green = ChannelIndexTrait<8, 1>;
    using Blue  = ChannelIndexTrait<8, 2>;
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_DXT1>
{
    using Pixel = void;
    using PaletteEntry = void;

    using Alpha = ChannelTrait<1>;
    using Red   = ChannelBitOffsetTrait<5, 11>;
    using Green = ChannelBitOffsetTrait<6, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;

    struct Block
    {
        uint16_t color[2];
        uint32_t lut;
    };
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_DXT3>
{
    using Pixel = void;
    using PaletteEntry = void;

    using Alpha = ChannelTrait<4>;
    using Red   = ChannelBitOffsetTrait<5, 11>;
    using Green = ChannelBitOffsetTrait<6, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;

    struct Block
    {
        uint64_t alpha_lut;
        uint16_t color[2];
        uint32_t lut;
    };
};

template<>
struct PixelFormatTrait<rf::BM_FORMAT_DXT5>
{
    using Pixel = void;
    using PaletteEntry = void;

    using Alpha = ChannelTrait<8>;
    using Red   = ChannelBitOffsetTrait<5, 11>;
    using Green = ChannelBitOffsetTrait<6, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;

    struct Block
    {
        uint8_t alpha[2];
        uint16_t alpha_lut[3];
        uint16_t color[2];
        uint32_t lut;
    };
};

template<rf::BmFormat FMT>
struct PixelColor
{
    ColorChannel<PixelFormatTrait<FMT>::Alpha::bits> a;
    ColorChannel<PixelFormatTrait<FMT>::Red::bits> r;
    ColorChannel<PixelFormatTrait<FMT>::Green::bits> g;
    ColorChannel<PixelFormatTrait<FMT>::Blue::bits> b;

    template<rf::BmFormat PF2>
    operator PixelColor<PF2>() const
    {
        return PixelColor<PF2>{a, r, g, b};
    }

    PixelColor<FMT> operator-(PixelColor<FMT> other) const
    {
        return PixelColor<FMT>{a - other.a, r - other.r, g - other.g, b - other.b};
    }

    PixelColor<FMT> operator+(PixelColor<FMT> other) const
    {
        return PixelColor<FMT>{a + other.a, r + other.r, g + other.g, b + other.b};
    }

    PixelColor<FMT> operator*(int n) const
    {
        return PixelColor<FMT>{a * n, r * n, g * n, b * n};
    }

    PixelColor<FMT> operator/(int n) const
    {
        return PixelColor<FMT>{a / n, r / n, g / n, b / n};
    }
};

template<typename CH, typename T>
ColorChannel<CH::bits> unpack_color_channel(T val)
{
    if constexpr (CH::bits == 0) {
        (void) val; // unused
        return {0};
    }
    else if constexpr (std::is_array_v<T>) {
        return {val[CH::index]};
    }
    else {
        constexpr int mask = (1 << CH::bits) - 1;
        return {static_cast<int>(val >> CH::offset) & mask};
    }
}

template<rf::BmFormat FMT>
PixelColor<FMT> unpack_color(typename PixelFormatTrait<FMT>::Pixel value)
{
    using PFT = PixelFormatTrait<FMT>;
    return {
        unpack_color_channel<typename PFT::Alpha>(value),
        unpack_color_channel<typename PFT::Red>(value),
        unpack_color_channel<typename PFT::Green>(value),
        unpack_color_channel<typename PFT::Blue>(value),
    };
}

template<rf::BmFormat>
class BlockDecoder;

template<>
class BlockDecoder<rf::BM_FORMAT_DXT1>
{
    using PFT = PixelFormatTrait<rf::BM_FORMAT_DXT1>;
    using Block = PFT::Block;
    const Block* block_;

public:
    BlockDecoder(const Block* block) : block_(block) {}

    PixelColor<rf::BM_FORMAT_DXT1> decode(int x, int y)
    {
        auto index = y * 4 + x;
        auto c0_u16 = block_->color[0];
        auto c1_u16 = block_->color[1];
        auto c0 = unpack_color<rf::BM_FORMAT_565_RGB>(c0_u16);
        auto c1 = unpack_color<rf::BM_FORMAT_565_RGB>(c1_u16);
        auto lut_val = (block_->lut >> (index * 2)) & 3;
        if (c0_u16 > c1_u16) {
            switch (lut_val) {
                case 0: return c0;
                case 1: return c1;
                case 2: return (c0 * 2 + c1) / 3;
                case 3: return (c0 + c1 * 2) / 3;
            }
        }
        else {
            switch (lut_val) {
                case 0: return c0;
                case 1: return c1;
                case 2: return (c0 + c1) / 2;
                case 3: return {}; // transparent
            }
        }
        // unreachable...
        return {};
    }
};

template<>
class BlockDecoder<rf::BM_FORMAT_DXT3>
{
    using PFT = PixelFormatTrait<rf::BM_FORMAT_DXT3>;
    using Block = PFT::Block;
    const Block* block_;

public:
    BlockDecoder(const Block* block) : block_(block) {}

    PixelColor<rf::BM_FORMAT_DXT3> decode(int x, int y)
    {
        auto index = y * 4 + x;
        auto a = static_cast<int>((block_->alpha_lut >> (index * 4)) & 15);
        auto c = decode_color(index);
        return {a, c.r, c.g, c.b};
    }

private:
    PixelColor<rf::BM_FORMAT_565_RGB> decode_color(int index)
    {
        auto c0 = unpack_color<rf::BM_FORMAT_565_RGB>(block_->color[0]);
        auto c1 = unpack_color<rf::BM_FORMAT_565_RGB>(block_->color[1]);
        auto lut_val = (block_->lut >> (index * 2)) & 3;
        switch (lut_val) {
            case 0: return c0;
            case 1: return c1;
            case 2: return (c0 * 2 + c1) / 3;
            case 3: return (c0 + c1 * 2) / 3;
        }
        // unreachable...
        return {};
    }
};

template<>
class BlockDecoder<rf::BM_FORMAT_DXT5>
{
    using PFT = PixelFormatTrait<rf::BM_FORMAT_DXT5>;
    using Block = PFT::Block;
    const Block* block_;

public:
    BlockDecoder(const Block* block) : block_(block) {}

    PixelColor<rf::BM_FORMAT_DXT5> decode(int x, int y)
    {
        auto index = y * 4 + x;
        auto a = decode_alpha(index);
        auto c = decode_color(index);
        return {a, c.r, c.g, c.b};
    }

private:
    int decode_alpha(int index)
    {
        auto a0 = block_->alpha[0];
        auto a1 = block_->alpha[1];
        auto alpha_lut_64 = static_cast<uint64_t>(block_->alpha_lut[2]) << 32
            | (static_cast<uint64_t>(block_->alpha_lut[1]) << 16)
            | static_cast<uint64_t>(block_->alpha_lut[0]);
        auto lut_val = (alpha_lut_64 >> (index * 3)) & 7;
        if (a0 > a1) {
            switch (lut_val) {
                case 0: return a0;
                case 1: return a1;
                case 2: return (6 * a0 + 1 * a1) / 7;
                case 3: return (5 * a0 + 2 * a1) / 7;
                case 4: return (4 * a0 + 3 * a1) / 7;
                case 5: return (3 * a0 + 4 * a1) / 7;
                case 6: return (2 * a0 + 5 * a1) / 7;
                case 7: return (1 * a0 + 6 * a1) / 7;
            }
        }
        else {
            switch (lut_val) {
                case 0: return a0;
                case 1: return a1;
                case 2: return (4 * a0 + 1 * a1) / 5;
                case 3: return (3 * a0 + 2 * a1) / 5;
                case 4: return (2 * a0 + 3 * a1) / 5;
                case 5: return (1 * a0 + 4 * a1) / 5;
                case 6: return 0;
                case 7: return 255;
            }
        }
        // unreachable...
        return {};
    }

    PixelColor<rf::BM_FORMAT_565_RGB> decode_color(int index)
    {
        auto c0 = unpack_color<rf::BM_FORMAT_565_RGB>(block_->color[0]);
        auto c1 = unpack_color<rf::BM_FORMAT_565_RGB>(block_->color[1]);
        auto lut_val = (block_->lut >> (index * 2)) & 3;
        switch (lut_val) {
            case 0: return c0;
            case 1: return c1;
            case 2: return (c0 * 2 + c1) / 3;
            case 3: return (c0 + c1 * 2) / 3;
        }
        // unreachable...
        return {};
    }
};

template<rf::BmFormat Fmt>
rf::Color DecodeBlockCompressedPixelInternal(const void* block, int x, int y)
{
    using PFT = PixelFormatTrait<Fmt>;
    using Block = typename PFT::Block;
    auto ptr = reinterpret_cast<const Block*>(block);
    auto blk_dec = BlockDecoder<Fmt>{ptr};
    PixelColor<rf::BmFormat::BM_FORMAT_8888_ARGB> color = blk_dec.decode(x, y);
    return {
        static_cast<uint8_t>(color.r.value),
        static_cast<uint8_t>(color.g.value),
        static_cast<uint8_t>(color.b.value),
        static_cast<uint8_t>(color.a.value),
    };
}

rf::Color decode_block_compressed_pixel(void* block, rf::BmFormat format, int x, int y)
{
    switch (format) {
        case rf::BM_FORMAT_DXT1:
            return DecodeBlockCompressedPixelInternal<rf::BM_FORMAT_DXT1>(block, x, y);
        case rf::BM_FORMAT_DXT3:
            return DecodeBlockCompressedPixelInternal<rf::BM_FORMAT_DXT3>(block, x, y);
        case rf::BM_FORMAT_DXT5:
            return DecodeBlockCompressedPixelInternal<rf::BM_FORMAT_DXT5>(block, x, y);
        // lets skip DXT2 and DXT4 for now because they are unpopular
        default:
            return rf::Color{0, 0, 0, 0};
    };
}

template<rf::BmFormat PF>
class PixelsReader
{
    using PFT = PixelFormatTrait<PF>;
    const typename PFT::Pixel* ptr_;
    const typename PFT::PaletteEntry* palette_;

    template<typename CH, typename T>
    ColorChannel<CH::bits> get_channel(const T& val)
    {
        if constexpr (CH::bits == 0) {
            return {0};
        }
        else if constexpr (std::is_array_v<T>) {
            return {val[CH::index]};
        }
        else {
            constexpr int mask = (1 << CH::bits) - 1;
            return {static_cast<int>(val >> CH::offset) & mask};
        }
    }

    template<typename T>
    PixelColor<PF> get_color(const T& val)
    {
        return {
            get_channel<typename PFT::Alpha>(val),
            get_channel<typename PFT::Red>(val),
            get_channel<typename PFT::Green>(val),
            get_channel<typename PFT::Blue>(val)
        };
    }

public:
    PixelsReader(const void* ptr, const void* palette = nullptr) :
        ptr_(reinterpret_cast<const typename PFT::Pixel*>(ptr)),
        palette_(reinterpret_cast<const typename PFT::PaletteEntry*>(palette))
    {}

    PixelColor<PF> read()
    {
        const auto& val = *(ptr_++);
        if constexpr (std::is_void_v<typename PFT::PaletteEntry>) {
            return get_color(val);
        }
        else {
            return get_color(palette_[val]);
        }
    }
};

template<rf::BmFormat PF>
class PixelsWriter
{
    using PFT = PixelFormatTrait<PF>;
    typename PixelFormatTrait<PF>::Pixel* ptr_;

    template<typename CH>
    void set_channel([[ maybe_unused ]] ColorChannel<CH::bits> val)
    {
        if constexpr (CH::bits == 0) {
            // nothing to do
        }
        else {
            (*ptr_)[CH::index] = val.value;
        }
    }

public:
    PixelsWriter(void* ptr) :
        ptr_(reinterpret_cast<typename PixelFormatTrait<PF>::Pixel*>(ptr))
    {}

    void write([[ maybe_unused ]] PixelColor<PF> color)
    {
        if constexpr (std::is_void_v<typename PFT::PaletteEntry>) {
            if constexpr (!std::is_array_v<typename PFT::Pixel>) {
                *ptr_ =
                    (color.a.value << PFT::Alpha::offset) |
                    (color.r.value << PFT::Red::offset) |
                    (color.g.value << PFT::Green::offset) |
                    (color.b.value << PFT::Blue::offset);
            }
            else {
                set_channel<typename PFT::Alpha>(color.a);
                set_channel<typename PFT::Red>(color.r);
                set_channel<typename PFT::Green>(color.g);
                set_channel<typename PFT::Blue>(color.b);
            }
            ptr_++;
        }
        else {
            throw std::runtime_error{"writing indexed pixels is not supported"};
        }
    }
};

template<rf::BmFormat SRC_FMT, rf::BmFormat DST_FMT>
class SurfacePixelFormatConverter
{
    const uint8_t* src_ptr_;
    uint8_t* dst_ptr_;
    size_t src_pitch_;
    size_t dst_pitch_;
    size_t w_;
    size_t h_;
    const void* src_palette_;

public:
    SurfacePixelFormatConverter(const void* src_ptr, void* dst_ptr, size_t src_pitch, size_t dst_pitch, size_t w, size_t h, const void* src_palette) :
        src_ptr_(reinterpret_cast<const uint8_t*>(src_ptr)), dst_ptr_(reinterpret_cast<uint8_t*>(dst_ptr)),
        src_pitch_(src_pitch), dst_pitch_(dst_pitch), w_(w), h_(h), src_palette_(src_palette)
    {}

    void operator()()
    {
#if TEXTURE_DITHERING
        auto prev_row_deltas = std::make_unique<PixelColor<SRC_FMT>[]>(w_ + 2);
        auto cur_row_deltas = std::make_unique<PixelColor<SRC_FMT>[]>(w_ + 2);
        while (h_ > 0) {
            PixelsReader<SRC_FMT> rdr{src_ptr_, src_palette_};
            PixelsWriter<DST_FMT> wrt{dst_ptr_};
            PixelColor<SRC_FMT> delta;
            for (size_t x = 0; x < w_; ++x) {
                PixelColor<SRC_FMT> old_clr = rdr.read()
                    + delta                  * 7 / 16
                    + prev_row_deltas[x]     * 3 / 16
                    + prev_row_deltas[x + 1] * 5 / 16
                    + prev_row_deltas[x + 2] * 1 / 16;
                // TODO: make sure values are in range?
                PixelColor<DST_FMT> new_clr = old_clr;
                delta = old_clr - static_cast<PixelColor<SRC_FMT>>(new_clr);
                cur_row_deltas[x + 1] = delta;
                wrt.write(new_clr);
            }
            --h_;
            src_ptr_ += src_pitch_;
            dst_ptr_ += dst_pitch_;
            cur_row_deltas.swap(prev_row_deltas);
        }
#else
        while (h_ > 0) {
            PixelsReader<SRC_FMT> rdr{src_ptr_, src_palette_};
            PixelsWriter<DST_FMT> wrt{dst_ptr_};
            for (size_t x = 0; x < w_; ++x) {
                wrt.write(rdr.read());
            }
            --h_;
            src_ptr_ += src_pitch_;
            dst_ptr_ += dst_pitch_;
        }
#endif
    }
};

template<rf::BmFormat FMT>
struct PixelFormatHolder
{
    static constexpr rf::BmFormat value = FMT;
};

template<typename F>
void CallWithPixelFormat(rf::BmFormat pixel_fmt, F handler)
{
    switch (pixel_fmt) {
        case rf::BM_FORMAT_8888_ARGB:
            handler(PixelFormatHolder<rf::BM_FORMAT_8888_ARGB>());
            return;
        case rf::BM_FORMAT_888_RGB:
            handler(PixelFormatHolder<rf::BM_FORMAT_888_RGB>());
            return;
        case rf::BM_FORMAT_565_RGB:
            handler(PixelFormatHolder<rf::BM_FORMAT_565_RGB>());
            return;
        case rf::BM_FORMAT_4444_ARGB:
            handler(PixelFormatHolder<rf::BM_FORMAT_4444_ARGB>());
            return;
        case rf::BM_FORMAT_1555_ARGB:
            handler(PixelFormatHolder<rf::BM_FORMAT_1555_ARGB>());
            return;
        case rf::BM_FORMAT_8_ALPHA:
            handler(PixelFormatHolder<rf::BM_FORMAT_8_ALPHA>());
            return;
        case rf::BM_FORMAT_8_PALETTED:
            handler(PixelFormatHolder<rf::BM_FORMAT_8_PALETTED>());
            return;
        case rf::BM_FORMAT_888_BGR:
            handler(PixelFormatHolder<rf::BM_FORMAT_888_BGR>());
            return;
        default:
            throw std::runtime_error{"Unhandled pixel format"};
    }
}

bool conver_surface_format(void* dst_bits_ptr, rf::BmFormat dst_fmt, const void* src_bits_ptr,
                               rf::BmFormat src_fmt, int width, int height, int dst_pitch, int src_pitch,
                               const uint8_t* palette)
{
#if DEBUG_PERF
    static auto& color_conv_perf = PerfAggregator::create("conver_surface_format");
    ScopedPerfMonitor mon{color_conv_perf};
#endif
    try {
        CallWithPixelFormat(src_fmt, [=](auto s) {
            CallWithPixelFormat(dst_fmt, [=](auto d) {
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

CodeInjection level_load_lightmaps_color_conv_patch{
    0x004ED3E9,
    [](auto& regs) {
        // Always skip original code
        regs.eip = 0x004ED4FA;

        auto lightmap = reinterpret_cast<rf::GLightmap*>(regs.ebx);

        rf::GrLockInfo lock;
        if (!rf::gr_lock(lightmap->bm_handle, 0, &lock, rf::GR_LOCK_WRITE_ONLY))
            return;

    #if 1 // cap minimal color channel value as RF does
        for (int i = 0; i < lightmap->w * lightmap->h * 3; ++i)
            lightmap->buf[i] = std::max(lightmap->buf[i], (uint8_t)(4 << 3)); // 32
    #endif

        bool success = conver_surface_format(lock.data, lock.format, lightmap->buf,
            rf::BM_FORMAT_888_BGR, lightmap->w, lightmap->h, lock.stride_in_bytes, 3 * lightmap->w, nullptr);
        if (!success)
            xlog::error("ConvertBitmapFormat failed for lightmap (dest format %d)", lock.format);

        rf::gr_unlock(&lock);
    },
};

CodeInjection GSurface_calculate_lightmap_color_conv_patch{
    0x004F2F23,
    [](auto& regs) {
        // Always skip original code
        regs.eip = 0x004F3023;

        auto face_light_info = reinterpret_cast<void*>(regs.esi);
        rf::GLightmap& lightmap = *struct_field_ref<rf::GLightmap*>(face_light_info, 12);
        rf::GrLockInfo lock;
        if (!rf::gr_lock(lightmap.bm_handle, 0, &lock, rf::GR_LOCK_WRITE_ONLY)) {
            return;
        }

        int offset_y = struct_field_ref<int>(face_light_info, 20);
        int offset_x = struct_field_ref<int>(face_light_info, 16);
        int src_width = lightmap.w;
        int dst_pixel_size = get_bm_format_size(lock.format);
        uint8_t* src_data = lightmap.buf + 3 * (offset_x + offset_y * src_width);
        uint8_t* dst_data = &lock.data[dst_pixel_size * offset_x + offset_y * lock.stride_in_bytes];
        int height = struct_field_ref<int>(face_light_info, 28);
        int src_pitch = 3 * src_width;
        bool success = conver_surface_format(dst_data, lock.format, src_data, rf::BM_FORMAT_888_BGR,
            src_width, height, lock.stride_in_bytes, src_pitch);
        if (!success)
            xlog::error("conver_surface_format failed for geomod (fmt %d)", lock.format);
        rf::gr_unlock(&lock);
    },
};

CodeInjection GSurface_alloc_lightmap_color_conv_patch{
    0x004E487B,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E4993;

        auto face_light_info = reinterpret_cast<void*>(regs.esi);
        rf::GLightmap& lightmap = *struct_field_ref<rf::GLightmap*>(face_light_info, 12);
        rf::GrLockInfo lock;
        if (!rf::gr_lock(lightmap.bm_handle, 0, &lock, rf::GR_LOCK_WRITE_ONLY)) {
            return;
        }

        int offset_y = struct_field_ref<int>(face_light_info, 20);
        int src_width = lightmap.w;
        int offset_x = struct_field_ref<int>(face_light_info, 16);
        uint8_t* src_data_begin = lightmap.buf;
        int src_offset = 3 * (offset_x + src_width * struct_field_ref<int>(face_light_info, 20)); // src offset
        uint8_t* src_data = src_offset + src_data_begin;
        int height = struct_field_ref<int>(face_light_info, 28);
        int dst_pixel_size = get_bm_format_size(lock.format);
        uint8_t* dst_row_ptr = &lock.data[dst_pixel_size * offset_x + offset_y * lock.stride_in_bytes];
        int src_pitch = 3 * src_width;
        bool success = conver_surface_format(dst_row_ptr, lock.format, src_data, rf::BM_FORMAT_888_BGR,
                                                 src_width, height, lock.stride_in_bytes, src_pitch);
        if (!success)
            xlog::error("ConvertBitmapFormat failed for geomod2 (fmt %d)", lock.format);
        rf::gr_unlock(&lock);
    },
};

CodeInjection g_proctex_update_water_patch{
    0x004E68D1,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E6B68;

        uintptr_t waveform_info = static_cast<uintptr_t>(regs.esi);
        int src_bm_handle = *reinterpret_cast<int*>(waveform_info + 36);
        rf::GrLockInfo src_lock_data, dst_lock_data;
        if (!rf::gr_lock(src_bm_handle, 0, &src_lock_data, rf::GR_LOCK_READ_ONLY)) {
            return;
        }
        int dst_bm_handle = *reinterpret_cast<int*>(waveform_info + 24);
        if (!rf::gr_lock(dst_bm_handle, 0, &dst_lock_data, rf::GR_LOCK_WRITE_ONLY)) {
            rf::gr_unlock(&src_lock_data);
            return;
        }

        try {
            CallWithPixelFormat(src_lock_data.format, [=](auto s) {
                CallWithPixelFormat(dst_lock_data.format, [=](auto d) {
                    auto& byte_1370f90 = addr_as_ref<uint8_t[256]>(0x1370F90);
                    auto& byte_1371b14 = addr_as_ref<uint8_t[256]>(0x1371B14);
                    auto& byte_1371090 = addr_as_ref<uint8_t[512]>(0x1371090);

                    uint8_t* dst_row_ptr = dst_lock_data.data;
                    int src_pixel_size = get_bm_format_size(src_lock_data.format);

                    for (int y = 0; y < dst_lock_data.h; ++y) {
                        int t1 = byte_1370f90[y];
                        int t2 = byte_1371b14[y];
                        uint8_t* off_arr = &byte_1371090[-t1];
                        PixelsWriter<decltype(d)::value> wrt{dst_row_ptr};
                        for (int x = 0; x < dst_lock_data.w; ++x) {
                            int src_x = t1;
                            int src_y = t2 + off_arr[t1];
                            int src_x_limited = src_x & (dst_lock_data.w - 1);
                            int src_y_limited = src_y & (dst_lock_data.h - 1);
                            const uint8_t* src_ptr = src_lock_data.data + src_x_limited * src_pixel_size + src_y_limited * src_lock_data.stride_in_bytes;
                            PixelsReader<decltype(s)::value> rdr{src_ptr};
                            wrt.write(rdr.read());
                            ++t1;
                        }
                        dst_row_ptr += dst_lock_data.stride_in_bytes;
                    }
                });
            });
        }
        catch (const std::exception& e) {
            xlog::error("Pixel format conversion failed for liquid wave texture: %s", e.what());
        }

        rf::gr_unlock(&src_lock_data);
        rf::gr_unlock(&dst_lock_data);
    }
};

CodeInjection GSolid_get_ambient_color_from_lightmap_patch{
    0x004E5CE3,
    [](auto& regs) {
        // Skip original code
        regs.eip = 0x004E5D57;

        int x = regs.edi;
        int y = regs.ebx;
        auto& lm = *reinterpret_cast<rf::GLightmap*>(regs.esi);
        auto& color = *reinterpret_cast<rf::Color*>(regs.esp + 0x34 - 0x28);

        // Optimization: instead of locking the lightmap texture get color data from lightmap pixels stored in RAM
        const uint8_t* src_ptr = lm.buf + (y * lm.w + x) * 3;
        color.set(src_ptr[0], src_ptr[1], src_ptr[2], 255);
    },
};

FunHook<unsigned()> bink_init_device_info_hook{
    0x005210C0,
    []() {
        unsigned bink_flags = bink_init_device_info_hook.call_target();
        const int BINKSURFACE32 = 3;

        if (g_game_config.true_color_textures && g_game_config.res_bpp == 32) {
            static auto& bink_bm_pixel_fmt = addr_as_ref<uint32_t>(0x018871C0);
            bink_bm_pixel_fmt = rf::BM_FORMAT_888_RGB;
            bink_flags = BINKSURFACE32;
        }

        return bink_flags;
    },
};

CodeInjection MonitorUpdateStatic_patch{
    0x004123AD,
    [](auto& regs) {
        // Note: default noise generation algohritm is not used because it's not uniform enough in high resolution
        static int noise_buf;
        if (regs.edx % 15 == 0)
            noise_buf = std::rand();
        bool white = noise_buf & 1;
        noise_buf >>= 1;

        auto& lock = *reinterpret_cast<rf::GrLockInfo*>(regs.esp + 0x2C - 0x20);
        auto pixel_ptr = reinterpret_cast<char*>(regs.esi);
        int bytes_per_pixel = get_bm_format_size(lock.format);

        std::fill(pixel_ptr, pixel_ptr + bytes_per_pixel, white ? '\0' : '\xFF');
        regs.esi += bytes_per_pixel;
        ++regs.edx;
        regs.eip = 0x004123DA;
    },
};

CodeInjection MonitorUpdateOff_patch{
    0x00412430,
    [](auto& regs) {
        auto& lock = *reinterpret_cast<rf::GrLockInfo*>(regs.esp + 0x34 - 0x20);
        regs.ecx = lock.h * lock.stride_in_bytes;
        regs.eip = 0x0041243F;
    },
};

void gr_color_init()
{
    // True Color textures
    if (g_game_config.res_bpp == 32 && g_game_config.true_color_textures) {
        // Available texture formats (tested for compatibility)
        write_mem<u32>(0x005A7DFC, D3DFMT_X8R8G8B8); // old: D3DFMT_R5G6B5
        write_mem<u32>(0x005A7E00, D3DFMT_A8R8G8B8); // old: D3DFMT_X1R5G5B5
        write_mem<u32>(0x005A7E04, D3DFMT_A8R8G8B8); // old: D3DFMT_A1R5G5B5, lightmaps
        write_mem<u32>(0x005A7E08, D3DFMT_A8R8G8B8); // old: D3DFMT_A4R4G4B4
        write_mem<u32>(0x005A7E0C, D3DFMT_A4R4G4B4); // old: D3DFMT_A8R3G3B2

        // use 32-bit texture for Bink rendering
        bink_init_device_info_hook.install();
    }

    // lightmaps
    level_load_lightmaps_color_conv_patch.install();
    // geomod
    GSurface_calculate_lightmap_color_conv_patch.install();
    GSurface_alloc_lightmap_color_conv_patch.install();
    // water
    AsmWriter(0x004E68B0, 0x004E68B6).nop();
    g_proctex_update_water_patch.install();
    // ambient color
    GSolid_get_ambient_color_from_lightmap_patch.install();
    // fix pixel format for lightmaps
    write_mem<u8>(0x004F5EB8 + 1, rf::BM_FORMAT_888_RGB);

    // monitor noise
    MonitorUpdateStatic_patch.install();
    // monitor off state
    MonitorUpdateOff_patch.install();

}
