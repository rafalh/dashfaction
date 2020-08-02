#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <common/version.h>
#include <xlog/xlog.h>
#include <cstring>
#include "../rf/ui.h"
#include "../rf/graphics.h"
#include "../rf/input.h"
#include "../rf/fs.h"
#include "../rf/network.h"
#include "../main.h"

#define DEBUG_UI_LAYOUT 0
#define SHARP_UI_TEXT 1

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

constexpr double PI = 3.14159265358979323846;

int g_version_click_counter = 0;
int g_egg_anim_start;

namespace rf
{
    static auto& menu_version_label = AddrAsRef<UiGadget>(0x0063C088);
}

// Note: fastcall is used because MSVC does not allow free thiscall functions
using UiLabel_Create2_Type = void __fastcall(rf::UiGadget*, void*, rf::UiGadget*, int, int, int, int, const char*, int);
extern CallHook<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_hook;
void __fastcall UiLabel_Create2_VersionLabel(rf::UiGadget* self, void* edx, rf::UiGadget* parent, int x, int y, int w,
                                             int h, const char* text, int font_id)
{
    text = PRODUCT_NAME_VERSION;
    rf::GrGetTextWidth(&w, &h, text, -1, font_id);
#if SHARP_UI_TEXT
    w = static_cast<int>(w / rf::ui_scale_x);
    h = static_cast<int>(h / rf::ui_scale_y);
#endif
    x = 430 - w;
    w += 5;
    h += 2;
    UiLabel_Create2_VersionLabel_hook.CallTarget(self, edx, parent, x, y, w, h, text, font_id);
}
CallHook<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_hook{0x0044344D, UiLabel_Create2_VersionLabel};

CallHook<void()> MenuMainProcessMouse_hook{
    0x004437B9,
    []() {
        MenuMainProcessMouse_hook.CallTarget();
        if (rf::MouseWasButtonPressed(0)) {
            int x, y, z;
            rf::MouseGetPos(x, y, z);
            rf::UiGadget* gadgets_to_check[1] = {&rf::menu_version_label};
            int matched = rf::UiGetGadgetFromPos(x, y, gadgets_to_check, std::size(gadgets_to_check));
            if (matched == 0) {
                xlog::trace("Version clicked");
                ++g_version_click_counter;
                if (g_version_click_counter == 3)
                    g_egg_anim_start = GetTickCount();
            }
        }
    },
};

int LoadEasterEggImage()
{
    HRSRC res_handle = FindResourceA(g_hmodule, MAKEINTRESOURCEA(100), RT_RCDATA);
    if (!res_handle) {
        xlog::error("FindResourceA failed");
        return -1;
    }
    HGLOBAL res_data_handle = LoadResource(g_hmodule, res_handle);
    if (!res_data_handle) {
        xlog::error("LoadResource failed");
        return -1;
    }
    void* res_data = LockResource(res_data_handle);
    if (!res_data) {
        xlog::error("LockResource failed");
        return -1;
    }

    constexpr int easter_egg_size = 128;

    int hbm = rf::BmCreateUserBmap(rf::BMPF_ARGB_8888, easter_egg_size, easter_egg_size);

    rf::GrLockData lock_data;
    if (!rf::GrLock(hbm, 0, &lock_data, 1))
        return -1;

    rf::BmConvertFormat(lock_data.bits, lock_data.pixel_format, res_data, rf::BMPF_ARGB_8888,
                        easter_egg_size * easter_egg_size);
    rf::GrUnlock(&lock_data);

    return hbm;
}

CallHook<void()> MenuMainRender_hook{
    0x00443802,
    []() {
        MenuMainRender_hook.CallTarget();
        if (g_version_click_counter >= 3) {
            static int img = LoadEasterEggImage(); // data.vpp
            if (img == -1)
                return;
            int w, h;
            rf::BmGetBitmapSize(img, &w, &h);
            int anim_delta_time = GetTickCount() - g_egg_anim_start;
            int pos_x = (rf::GrGetMaxWidth() - w) / 2;
            int pos_y = rf::GrGetMaxHeight() - h;
            if (anim_delta_time < EGG_ANIM_ENTER_TIME) {
                float enter_progress = anim_delta_time / static_cast<float>(EGG_ANIM_ENTER_TIME);
                pos_y += h - static_cast<int>(sinf(enter_progress * static_cast<float>(PI) / 2.0f) * h);
            }
            else if (anim_delta_time > EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME) {
                int leave_delta = anim_delta_time - (EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME);
                float leave_progress = leave_delta / static_cast<float>(EGG_ANIM_LEAVE_TIME);
                pos_y += static_cast<int>((1.0f - cosf(leave_progress * static_cast<float>(PI) / 2.0f)) * h);
                if (leave_delta > EGG_ANIM_LEAVE_TIME)
                    g_version_click_counter = 0;
            }
            rf::GrBitmap(img, pos_x, pos_y, rf::gr_bitmap_clamp_state);
        }
    },
};

struct ServerListEntry
{
    char name[32];
    char level_name[32];
    char mod_name[16];
    int game_type;
    rf::NwAddr addr;
    char current_players;
    char max_players;
    int16_t ping;
    int field_60;
    char field_64;
    int flags;
};
static_assert(sizeof(ServerListEntry) == 0x6C, "invalid size");

FunHook<int(const int&, const int&)> ServerListCmpFunc_hook{
    0x0044A6D0,
    [](const int& index1, const int& index2) {
        auto server_list = AddrAsRef<ServerListEntry*>(0x0063F62C);
        bool has_ping1 = server_list[index1].ping >= 0;
        bool has_ping2 = server_list[index2].ping >= 0;
        if (has_ping1 != has_ping2)
            return has_ping1 ? -1 : 1;
        else
            return ServerListCmpFunc_hook.CallTarget(index1, index2);
    },
};

static inline void DebugUiLayout([[ maybe_unused ]] rf::UiGadget& gadget)
{
#if DEBUG_UI_LAYOUT
    int x = gadget.GetAbsoluteX() * rf::ui_scale_x;
    int y = gadget.GetAbsoluteY() * rf::ui_scale_y;
    int w = gadget.w * rf::ui_scale_x;
    int h = gadget.h * rf::ui_scale_y;
    rf::GrSetColor((x ^ y) & 255, 0, 0, 64);
    rf::GrRect(x, y, w, h);
#endif
}

void __fastcall UiButton_Create(rf::UiButton& this_, void*, const char *normal_bm, const char *selected_bm, int x, int y, int id, const char *text, int font)
{
    this_.key = id;
    this_.x = x;
    this_.y = y;
    if (*normal_bm) {
        this_.bg_bitmap = rf::BmLoad(normal_bm, -1, false);
        rf::GrTcacheAddRef(this_.bg_bitmap);
        rf::BmGetBitmapSize(this_.bg_bitmap, &this_.w, &this_.h);
    }
    if (*selected_bm) {
        this_.selected_bitmap = rf::BmLoad(selected_bm, -1, false);
        rf::GrTcacheAddRef(this_.selected_bitmap);
        if (this_.bg_bitmap < 0) {
            rf::BmGetBitmapSize(this_.selected_bitmap, &this_.w, &this_.h);
        }
    }
    this_.text = strdup(text);
    this_.font = font;
}
FunHook UiButton_Create_hook{0x004574D0, UiButton_Create};

void __fastcall UiButton_SetText(rf::UiButton& this_, void*, const char *text, int font)
{
    delete[] this_.text;
    this_.text = strdup(text);
    this_.font = font;
}
FunHook UiButton_SetText_hook{0x00457710, UiButton_SetText};

void __fastcall UiButton_Render(rf::UiButton& this_, void*)
{
    int x = static_cast<int>(this_.GetAbsoluteX() * rf::ui_scale_x);
    int y = static_cast<int>(this_.GetAbsoluteY() * rf::ui_scale_y);
    int w = static_cast<int>(this_.w * rf::ui_scale_x);
    int h = static_cast<int>(this_.h * rf::ui_scale_y);

    if (this_.bg_bitmap >= 0) {
        rf::GrSetColor(255, 255, 255, 255);
        rf::GrBitmapStretched(this_.bg_bitmap, x, y, w, h, 0, 0, this_.w, this_.h);
    }

    if (!this_.enabled) {
        rf::GrSetColor(96, 96, 96, 255);
    }
    else if (this_.highlighted) {
        rf::GrSetColor(240, 240, 240, 255);
    }
    else {
        rf::GrSetColor(192, 192, 192, 255);
    }

    if (this_.enabled && this_.highlighted && this_.selected_bitmap >= 0) {
        auto state = AddrAsRef<rf::GrRenderState>(0x01775B0C);
        rf::GrBitmapStretched(this_.selected_bitmap, x, y, w, h, 0, 0, this_.w, this_.h, false, false, state);
    }

    // Change clip region for text rendering
    int clip_x, clip_y, clip_w, clip_h;
    rf::GrGetClip(&clip_x, &clip_y, &clip_w, &clip_h);
    rf::GrSetClip(x, y, w, h);

    std::string_view text_sv{this_.text};
    int num_lines = 1 + std::count(text_sv.begin(), text_sv.end(), '\n');
    int text_h = rf::GrGetFontHeight(this_.font) * num_lines;
    int text_y = (h - text_h) / 2;
    rf::GrString(rf::center_x, text_y, this_.text, this_.font);

    // Restore clip region
    rf::GrSetClip(clip_x, clip_y, clip_w, clip_h);

    DebugUiLayout(this_);
}
FunHook UiButton_Render_hook{0x004577A0, UiButton_Render};

void __fastcall UiLabel_Create(rf::UiLabel& this_, void*, rf::UiGadget *parent, int x, int y, const char *text, int font)
{
    this_.parent = parent;
    this_.x = x;
    this_.y = y;
    int text_w, text_h;
    rf::GrGetTextWidth(&text_w, &text_h, text, -1, font);
    this_.w = static_cast<int>(text_w / rf::ui_scale_x);
    this_.h = static_cast<int>(text_h / rf::ui_scale_y);
    this_.text = strdup(text);
    this_.font = font;
    this_.align = rf::GR_ALIGN_LEFT;
    this_.clr.SetRGBA(0, 0, 0, 255);
}
FunHook UiLabel_Create_hook{0x00456B60, UiLabel_Create};

void __fastcall UiLabel_Create2(rf::UiLabel& this_, void*, rf::UiGadget *parent, int x, int y, int w, int h, const char *text, int font)
{
    this_.parent = parent;
    this_.x = x;
    this_.y = y;
    this_.w = w;
    this_.h = h;
    if (*text == ' ') {
        while (*text == ' ') {
            ++text;
        }
        this_.align = rf::GR_ALIGN_CENTER;
    }
    else {
        this_.align = rf::GR_ALIGN_LEFT;
    }
    this_.text = strdup(text);
    this_.font = font;
    this_.clr.SetRGBA(0, 0, 0, 255);
}
FunHook UiLabel_Create2_hook{0x00456C20, UiLabel_Create2};

void __fastcall UiLabel_SetText(rf::UiLabel& this_, void*, const char *text, int font)
{
    delete[] this_.text;
    this_.text = strdup(text);
    this_.font = font;
}
FunHook UiLabel_SetText_hook{0x00456DC0, UiLabel_SetText};

void __fastcall UiLabel_Render(rf::UiLabel& this_, void*)
{
    if (!this_.enabled) {
        rf::GrSetColor(48, 48, 48, 128);
    }
    else if (this_.highlighted) {
        rf::GrSetColor(240, 240, 240, 255);
    }
    else {
        rf::GrSetColorPtr(&this_.clr);
    }
    int x = static_cast<int>(this_.GetAbsoluteX() * rf::ui_scale_x);
    int y = static_cast<int>(this_.GetAbsoluteY() * rf::ui_scale_y);
    int text_w, text_h;
    rf::GrGetTextWidth(&text_w, &text_h, this_.text, -1, this_.font);
    if (this_.align == rf::GR_ALIGN_CENTER) {
        x += static_cast<int>(this_.w * rf::ui_scale_x / 2);
    }
    else if (this_.align == rf::GR_ALIGN_RIGHT) {
        x += static_cast<int>(this_.w * rf::ui_scale_x);
    }
    else {
        x += static_cast<int>(1 * rf::ui_scale_x);
    }
    rf::GrStringAligned(this_.align, x, y, this_.text, this_.font);

    DebugUiLayout(this_);
}
FunHook UiLabel_Render_hook{0x00456ED0, UiLabel_Render};

void __fastcall UiInputBox_Create(rf::UiInputBox& this_, void*, rf::UiGadget *parent, int x, int y, const char *text, int font, int w)
{
    this_.parent = parent;
    this_.x = x;
    this_.y = y;
    this_.w = w;
    this_.h = static_cast<int>(rf::GrGetFontHeight(font) / rf::ui_scale_y);
    this_.max_text_width = static_cast<int>(w * rf::ui_scale_x);
    this_.font = font;
    std::strncpy(this_.text, text, std::size(this_.text));
    this_.text[std::size(this_.text) - 1] = '\0';
}
FunHook UiInputBox_Create_hook{0x00456FE0, UiInputBox_Create};

void __fastcall UiInputBox_Render(rf::UiInputBox& this_, void*)
{
    if (this_.enabled && this_.highlighted) {
        rf::GrSetColor(240, 240, 240, 255);
    }
    else {
        rf::GrSetColor(192, 192, 192, 255);
    }

    int x = static_cast<int>((this_.GetAbsoluteX() + 1) * rf::ui_scale_x);
    int y = static_cast<int>(this_.GetAbsoluteY() * rf::ui_scale_y);
    int clip_x, clip_y, clip_w, clip_h;
    rf::GrGetClip(&clip_x, &clip_y, &clip_w, &clip_h);
    rf::GrSetClip(x, y, this_.max_text_width, static_cast<int>(this_.h * rf::ui_scale_y + 5)); // for some reason input fields are too thin
    int text_offset_x = static_cast<int>(1 * rf::ui_scale_x);
    rf::GrString(text_offset_x, 0, this_.text, this_.font);

    if (this_.enabled && this_.highlighted) {
        rf::UiUpdateInputBoxCursor();
        if (rf::ui_input_box_cursor_visible) {
            int text_w, text_h;
            rf::GrGetTextWidth(&text_w, &text_h, this_.text, -1, this_.font);
            rf::GrString(text_offset_x + text_w, 0, "_", this_.font);
        }
    }
    rf::GrSetClip(clip_x, clip_y, clip_w, clip_h);

    DebugUiLayout(this_);
}
FunHook UiInputBox_Render_hook{0x004570E0, UiInputBox_Render};

void __fastcall UiCycler_AddItem(rf::UiCycler& this_, void*, const char *text, int font)
{
    if (this_.num_items < this_.max_items) {
        this_.items_text[this_.num_items] = strdup(text);
        this_.items_font[this_.num_items] = font;
        ++this_.num_items;
    }
}
FunHook UiCycler_AddItem_hook{0x00458080, UiCycler_AddItem};

void __fastcall UiCycler_Render(rf::UiCycler& this_, void*)
{
    if (this_.enabled && this_.highlighted) {
        rf::GrSetColor(255, 255, 255, 255);
    }
    else if (this_.enabled) {
        rf::GrSetColor(192, 192, 192, 255);
    }
    else {
        rf::GrSetColor(96, 96, 96, 255);
    }

    int x = static_cast<int>(this_.GetAbsoluteX() * rf::ui_scale_x);
    int y = static_cast<int>(this_.GetAbsoluteY() * rf::ui_scale_y);

    auto text = this_.items_text[this_.current_item];
    auto font = this_.items_font[this_.current_item];
    auto font_h = rf::GrGetFontHeight(font);
    auto text_x = x + static_cast<int>(this_.w * rf::ui_scale_x / 2);
    auto text_y = y + static_cast<int>((this_.h * rf::ui_scale_y - font_h) / 2);
    rf::GrStringAligned(rf::GR_ALIGN_CENTER, text_x, text_y, text, font);

    DebugUiLayout(this_);
}
FunHook UiCycler_Render_hook{0x00457F40, UiCycler_Render};

CallHook<void(int*, int*, const char*, int, int, char, int)> GrFitMultilineText_UiSetModalDlgText_hook{
    0x00455A7D,
    [](int *len_array, int *offset_array, const char *text, int max_width, int max_lines, char unk_char, int font) {
        max_width = static_cast<int>(max_width * rf::ui_scale_x);
        GrFitMultilineText_UiSetModalDlgText_hook.CallTarget(len_array, offset_array, text, max_width, max_lines, unk_char, font);
    },
};

FunHook<void()> MenuInit_hook{
    0x00442BB0,
    []() {
        MenuInit_hook.CallTarget();
#if SHARP_UI_TEXT
        xlog::info("UI scale: %.4f %.4f", rf::ui_scale_x, rf::ui_scale_y);
        if (rf::ui_scale_y > 1.0f) {
            int large_font_size = std::min(128, static_cast<int>(std::round(rf::ui_scale_y * 14.5f))); // 32
            int medium_font_size = std::min(128, static_cast<int>(std::round(rf::ui_scale_y * 9.0f))); // 20
            int small_font_size = std::min(128, static_cast<int>(std::round(rf::ui_scale_y * 7.5f))); // 16
            xlog::info("UI font sizes: %d %d %d", large_font_size, medium_font_size, small_font_size);

            rf::ui_large_font = rf::GrLoadFont(StringFormat("boldfont.ttf:%d", large_font_size).c_str());
            rf::ui_medium_font_0 = rf::GrLoadFont(StringFormat("regularfont.ttf:%d", medium_font_size).c_str());
            rf::ui_medium_font_1 = rf::ui_medium_font_0;
            rf::ui_small_font = rf::GrLoadFont(StringFormat("regularfont.ttf:%d", small_font_size).c_str());
        }
#endif
    },
};

int BmLoadIfExists(const char* name, int unk, bool generate_mipmaps)
{
    if (rf::FsFileExists(name)) {
        return rf::BmLoad(name, unk, generate_mipmaps);
    }
    else {
        return -1;
    }
}

CallHook<void(int, int, int, rf::GrRenderState)> GrBitmap_DrawCursor_hook{
    0x004354E4,
    [](int bm_handle, int x, int y, rf::GrRenderState render_state) {
        if (rf::ui_scale_y >= 2.0f) {
            static int cursor_1_bmh = BmLoadIfExists("cursor_1.tga", -1, false);
            if (cursor_1_bmh != -1) {
                bm_handle = cursor_1_bmh;
            }
        }
        GrBitmap_DrawCursor_hook.CallTarget(bm_handle, x, y, render_state);
    },
};

void ApplyMainMenuPatches()
{
    // Version in Main Menu
    UiLabel_Create2_VersionLabel_hook.Install();

    // Version Easter Egg
    MenuMainProcessMouse_hook.Install();
    MenuMainRender_hook.Install();

    // Put not responding servers at the bottom of server list
    ServerListCmpFunc_hook.Install();

    // Sharp UI text
#if SHARP_UI_TEXT
    UiButton_Create_hook.Install();
    UiButton_SetText_hook.Install();
    UiButton_Render_hook.Install();
    UiLabel_Create_hook.Install();
    UiLabel_Create2_hook.Install();
    UiLabel_SetText_hook.Install();
    UiLabel_Render_hook.Install();
    UiInputBox_Create_hook.Install();
    UiInputBox_Render_hook.Install();
    UiCycler_AddItem_hook.Install();
    UiCycler_Render_hook.Install();
    GrFitMultilineText_UiSetModalDlgText_hook.Install();
#endif

    // Init
    MenuInit_hook.Install();

    // Bigger cursor bitmap support
    GrBitmap_DrawCursor_hook.Install();
}
