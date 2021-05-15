#pragma once

#include <patch_common/MemUtils.h>
#include "gr/gr_font.h"

namespace rf::ui
{
    struct Gadget
    {
        void **vtbl;
        Gadget *parent;
        bool highlighted;
        bool enabled;
        int x;
        int y;
        int w;
        int h;
        int key;
        void(*on_click)();
        void(*on_mouse_btn_down)();

        [[nodiscard]] int get_absolute_x() const
        {
            return AddrCaller{0x004569E0}.this_call<int>(this);
        }

        [[nodiscard]] int get_absolute_y() const
        {
            return AddrCaller{0x00456A00}.this_call<int>(this);
        }
    };
    static_assert(sizeof(Gadget) == 0x28);

    struct Button : Gadget
    {
        int bg_bitmap;
        int selected_bitmap;
#ifdef DASH_FACTION
        int reserved0;
        char* text;
        int font;
        int reserved1[2];
#else
        int text_user_bmap;
        int text_offset_x;
        int text_offset_y;
        int text_width;
        int text_height;
#endif
    };
    static_assert(sizeof(Button) == 0x44);

    struct Label : Gadget
    {
        Color clr;
        int bitmap;
#ifdef DASH_FACTION
        char* text;
        int font;
        gr::TextAlignment align;
#else
        int text_offset_x;
        int text_offset_y;
        int text_width;
#endif
        int text_height;
    };
    static_assert(sizeof(Label) == 0x40);

    struct InputBox : Label
    {
        char text[32];
        int max_text_width;
        int font;
    };
    static_assert(sizeof(InputBox) == 0x68);

    struct Cycler : Gadget
    {
        static constexpr int max_items = 16;
        int item_text_bitmaps[max_items];
#ifdef DASH_FACTION
        char* items_text[max_items];
        int items_font[max_items];
#else
        int item_text_offset_x[max_items];
        int item_text_offset_y[max_items];
#endif
        int items_width[max_items];
        int items_height[max_items];
        int num_items;
        int current_item;
    };
    static_assert(sizeof(Cycler) == 0x170);

    static auto& popup_message = addr_as_ref<void(const char *title, const char *text, void(*callback)(), bool input)>(0x004560B0);
    using PopupCallbackPtr = void (*)();
    static auto& popup_custom =
        addr_as_ref<void(const char *title, const char *text, int num_btns, const char *choices[],
                       PopupCallbackPtr choices_callbacks[], int default_choice, int keys[])>(0x004562A0);
    static auto& popup_abort = addr_as_ref<void()>(0x004559C0);
    static auto& popup_set_text = addr_as_ref<void(const char *text)>(0x00455A50);

    static auto& get_gadget_from_pos = addr_as_ref<int(int x, int y, Gadget * const gadgets[], int num_gadgets)>(0x00442ED0);
    static auto& update_input_box_cursor = addr_as_ref<void()>(0x00456960);

    static auto& scale_x = addr_as_ref<float>(0x00598FB8);
    static auto& scale_y = addr_as_ref<float>(0x00598FBC);
    static auto& input_box_cursor_visible = addr_as_ref<bool>(0x00642DC8);
    static auto& large_font = addr_as_ref<int>(0x0063C05C);
    static auto& medium_font_0 = addr_as_ref<int>(0x0063C060);
    static auto& medium_font_1 = addr_as_ref<int>(0x0063C064);
    static auto& small_font = addr_as_ref<int>(0x0063C068);
}
