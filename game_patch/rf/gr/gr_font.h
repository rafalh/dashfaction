#pragma once

#include "gr.h"

namespace rf::gr
{
    enum TextAlignment
    {
        ALIGN_LEFT = 0,
        ALIGN_CENTER = 1,
        ALIGN_RIGHT = 2,
    };

    struct FontKernEntry
    {
        char ch0;
        char ch1;
        char offset;
    };

    struct FontCharInfo
    {
        int spacing;
        int width;
        int pixel_data_offset;
        short kerning_entry;
        short user_data;
    };

    struct Font
    {
        char filename[32];
        int signature;
        int format;
        int num_chars;
        int first_ascii;
        int default_spacing;
        int height;
        int height2;
        int num_kern_pairs;
        int pixel_data_size;
        FontKernEntry *kern_data;
        FontCharInfo *chars;
        unsigned char *pixel_data;
        int *palette;
        int bm_handle;
        int bm_width;
        int bm_height;
        int *char_x_coords;
        int *char_y_coords;
        unsigned char *bm_bits;
    };

    static constexpr int center_x = 0x8000; // supported by gr_string

    static auto& string_mode = addr_as_ref<Mode>(0x017C7C5C);
    static constexpr int max_fonts = 12;
    static auto& fonts = addr_as_ref<Font[max_fonts]>(0x01886C90);
    static auto& num_fonts = addr_as_ref<int>(0x018871A0);

    inline void string(int x, int y, const char *s, int font_num = -1, Mode mode = string_mode)
    {
        AddrCaller{0x0051FEB0}.c_call(x, y, s, font_num, mode);
    }

    inline void string_aligned(TextAlignment align, int x, int y, const char *s, int font_num = -1, Mode state = string_mode)
    {
        AddrCaller{0x0051FE50}.c_call(align, x, y, s, font_num, state);
    }

    inline int load_font(const char *file_name, int a2 = -1)
    {
        return AddrCaller{0x0051F6E0}.c_call<int>(file_name, a2);
    }

    inline int get_font_height(int font_num = -1)
    {
        return AddrCaller{0x0051F4D0}.c_call<int>(font_num);
    }

    static auto& split_str = addr_as_ref<int(int *len_array, int *offset_array, char *s, int max_w, int max_lines, char unk_char, int font_num)>(0x00520810);
    static auto& get_string_size = addr_as_ref<void(int *out_w, int *out_h, const char *s, int s_len, int font_num)>(0x0051F530);
    static auto& set_default_font = addr_as_ref<bool(const char *file_name)>(0x0051FE20);
}
