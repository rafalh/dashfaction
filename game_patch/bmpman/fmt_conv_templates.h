#pragma once

#include <stdexcept>
#include "../rf/bmpman.h"
#include "../rf/gr/gr.h"

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

template<rf::bm::Format PF>
struct PixelFormatTrait;

template<>
struct PixelFormatTrait<rf::bm::FORMAT_8888_ARGB>
{
    using Pixel = uint32_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<8, 24>;
    using Red   = ChannelBitOffsetTrait<8, 16>;
    using Green = ChannelBitOffsetTrait<8, 8>;
    using Blue  = ChannelBitOffsetTrait<8, 0>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_888_RGB>
{
    using Pixel = uint8_t[3];
    using PaletteEntry = void;

    using Alpha = ChannelIndexTrait<0, 0>;
    using Red   = ChannelIndexTrait<8, 2>;
    using Green = ChannelIndexTrait<8, 1>;
    using Blue  = ChannelIndexTrait<8, 0>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_1555_ARGB>
{
    using Pixel = uint16_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<1, 15>;
    using Red   = ChannelBitOffsetTrait<5, 10>;
    using Green = ChannelBitOffsetTrait<5, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_565_RGB>
{
    using Pixel = uint16_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<0, 0>;
    using Red   = ChannelBitOffsetTrait<5, 11>;
    using Green = ChannelBitOffsetTrait<6, 5>;
    using Blue  = ChannelBitOffsetTrait<5, 0>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_4444_ARGB>
{
    using Pixel = uint16_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<4, 12>;
    using Red   = ChannelBitOffsetTrait<4, 8>;
    using Green = ChannelBitOffsetTrait<4, 4>;
    using Blue  = ChannelBitOffsetTrait<4, 0>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_8_ALPHA>
{
    using Pixel = uint8_t;
    using PaletteEntry = void;

    using Alpha = ChannelBitOffsetTrait<8, 0>;
    using Red   = ChannelBitOffsetTrait<0, 0>;
    using Green = ChannelBitOffsetTrait<0, 0>;
    using Blue  = ChannelBitOffsetTrait<0, 0>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_8_PALETTED>
{
    using Pixel = uint8_t;
    using PaletteEntry = uint8_t[3];

    using Alpha = ChannelIndexTrait<0, 0>;
    using Red = ChannelIndexTrait<8, 0>;
    using Green = ChannelIndexTrait<8, 1>;
    using Blue = ChannelIndexTrait<8, 2>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_888_BGR>
{
    using Pixel = uint8_t[3];
    using PaletteEntry = void;

    using Alpha = ChannelIndexTrait<0, 0>;
    using Red   = ChannelIndexTrait<8, 0>;
    using Green = ChannelIndexTrait<8, 1>;
    using Blue  = ChannelIndexTrait<8, 2>;
};

template<>
struct PixelFormatTrait<rf::bm::FORMAT_DXT1>
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
struct PixelFormatTrait<rf::bm::FORMAT_DXT3>
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
struct PixelFormatTrait<rf::bm::FORMAT_DXT5>
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

template<rf::bm::Format FMT>
struct PixelColor
{
    ColorChannel<PixelFormatTrait<FMT>::Alpha::bits> a;
    ColorChannel<PixelFormatTrait<FMT>::Red::bits> r;
    ColorChannel<PixelFormatTrait<FMT>::Green::bits> g;
    ColorChannel<PixelFormatTrait<FMT>::Blue::bits> b;

    template<rf::bm::Format PF2>
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

template<rf::bm::Format FMT>
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

template<rf::bm::Format>
class BlockDecoder;

template<>
class BlockDecoder<rf::bm::FORMAT_DXT1>
{
    using PFT = PixelFormatTrait<rf::bm::FORMAT_DXT1>;
    using Block = PFT::Block;
    const Block* block_;

public:
    BlockDecoder(const Block* block) : block_(block) {}

    PixelColor<rf::bm::FORMAT_DXT1> decode(int x, int y)
    {
        auto index = y * 4 + x;
        auto c0_u16 = block_->color[0];
        auto c1_u16 = block_->color[1];
        auto c0 = unpack_color<rf::bm::FORMAT_565_RGB>(c0_u16);
        auto c1 = unpack_color<rf::bm::FORMAT_565_RGB>(c1_u16);
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
class BlockDecoder<rf::bm::FORMAT_DXT3>
{
    using PFT = PixelFormatTrait<rf::bm::FORMAT_DXT3>;
    using Block = PFT::Block;
    const Block* block_;

public:
    BlockDecoder(const Block* block) : block_(block) {}

    PixelColor<rf::bm::FORMAT_DXT3> decode(int x, int y)
    {
        auto index = y * 4 + x;
        auto a = static_cast<int>((block_->alpha_lut >> (index * 4)) & 15);
        auto c = decode_color(index);
        return {a, c.r, c.g, c.b};
    }

private:
    PixelColor<rf::bm::FORMAT_565_RGB> decode_color(int index)
    {
        auto c0 = unpack_color<rf::bm::FORMAT_565_RGB>(block_->color[0]);
        auto c1 = unpack_color<rf::bm::FORMAT_565_RGB>(block_->color[1]);
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
class BlockDecoder<rf::bm::FORMAT_DXT5>
{
    using PFT = PixelFormatTrait<rf::bm::FORMAT_DXT5>;
    using Block = PFT::Block;
    const Block* block_;

public:
    BlockDecoder(const Block* block) : block_(block) {}

    PixelColor<rf::bm::FORMAT_DXT5> decode(int x, int y)
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

    PixelColor<rf::bm::FORMAT_565_RGB> decode_color(int index)
    {
        auto c0 = unpack_color<rf::bm::FORMAT_565_RGB>(block_->color[0]);
        auto c1 = unpack_color<rf::bm::FORMAT_565_RGB>(block_->color[1]);
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

template<rf::bm::Format Fmt>
rf::Color decode_block_compressed_pixel_internal(const void* block, int x, int y)
{
    using PFT = PixelFormatTrait<Fmt>;
    using Block = typename PFT::Block;
    auto ptr = reinterpret_cast<const Block*>(block);
    auto blk_dec = BlockDecoder<Fmt>{ptr};
    PixelColor<rf::bm::FORMAT_8888_ARGB> color = blk_dec.decode(x, y);
    return {
        static_cast<uint8_t>(color.r.value),
        static_cast<uint8_t>(color.g.value),
        static_cast<uint8_t>(color.b.value),
        static_cast<uint8_t>(color.a.value),
    };
}

inline rf::Color decode_block_compressed_pixel(void* block, rf::bm::Format format, int x, int y)
{
    switch (format) {
        case rf::bm::FORMAT_DXT1:
            return decode_block_compressed_pixel_internal<rf::bm::FORMAT_DXT1>(block, x, y);
        case rf::bm::FORMAT_DXT3:
            return decode_block_compressed_pixel_internal<rf::bm::FORMAT_DXT3>(block, x, y);
        case rf::bm::FORMAT_DXT5:
            return decode_block_compressed_pixel_internal<rf::bm::FORMAT_DXT5>(block, x, y);
        // lets skip DXT2 and DXT4 for now because they are unpopular
        default:
            return rf::Color{0, 0, 0, 0};
    };
}

template<rf::bm::Format PF>
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

template<rf::bm::Format PF>
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

template<rf::bm::Format SRC_FMT, rf::bm::Format DST_FMT>
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

template<rf::bm::Format FMT>
struct PixelFormatHolder
{
    static constexpr rf::bm::Format value = FMT;
};

template<typename F, bool HandlePaletted = true>
void call_with_format(rf::bm::Format format, F handler)
{
    switch (format) {
        case rf::bm::FORMAT_8888_ARGB:
            handler(PixelFormatHolder<rf::bm::FORMAT_8888_ARGB>());
            return;
        case rf::bm::FORMAT_888_RGB:
            handler(PixelFormatHolder<rf::bm::FORMAT_888_RGB>());
            return;
        case rf::bm::FORMAT_565_RGB:
            handler(PixelFormatHolder<rf::bm::FORMAT_565_RGB>());
            return;
        case rf::bm::FORMAT_4444_ARGB:
            handler(PixelFormatHolder<rf::bm::FORMAT_4444_ARGB>());
            return;
        case rf::bm::FORMAT_1555_ARGB:
            handler(PixelFormatHolder<rf::bm::FORMAT_1555_ARGB>());
            return;
        case rf::bm::FORMAT_8_ALPHA:
            handler(PixelFormatHolder<rf::bm::FORMAT_8_ALPHA>());
            return;
        case rf::bm::FORMAT_8_PALETTED:
            if constexpr (HandlePaletted) {
                handler(PixelFormatHolder<rf::bm::FORMAT_8_PALETTED>());
            } else {
                throw std::runtime_error{"Unexpected paletted pixel format"};
            }
            return;
        case rf::bm::FORMAT_888_BGR:
            handler(PixelFormatHolder<rf::bm::FORMAT_888_BGR>());
            return;
        default:
            throw std::runtime_error{"Unhandled pixel format"};
    }
}
