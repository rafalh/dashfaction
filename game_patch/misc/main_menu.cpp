#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <common/version/version.h>
#include <xlog/xlog.h>
#include "../rf/ui.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/input.h"
#include "../rf/file/file.h"
#include "../rf/multi.h"
#include "../rf/sound/sound.h"
#include "../rf/os/frametime.h"
#include "../rf/os/os.h"
#include "../main/main.h"
#include "../graphics/gr.h"
#include "misc.h"

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

constexpr double PI = 3.14159265358979323846;

static int g_version_click_counter = 0;
static int g_egg_anim_start;
static int g_game_music_sig_to_restore = -1;

namespace rf
{
    static auto& menu_version_label = addr_as_ref<ui::Gadget>(0x0063C088);
    static auto& main_menu_music_sig = addr_as_ref<int>(0x005990F8);
}

// Note: fastcall is used because MSVC does not allow free thiscall functions
using UiLabel_Create2_Type = void __fastcall(rf::ui::Gadget*, int, rf::ui::Gadget*, int, int, int, int, const char*, int);
extern CallHook<UiLabel_Create2_Type> UiLabel_create2_version_label_hook;
void __fastcall UiLabel_create2_version_label(rf::ui::Gadget* self, int edx, rf::ui::Gadget* parent, int x, int y, int w,
                                             int h, const char* text, int font_id)
{
    // if a TC mod is loaded, show the mod name on the main menu
    std::string version_text = rf::mod_param.found()
        ? std::format("DF {} | {}", VERSION_STR, rf::mod_param.get_arg())
        : PRODUCT_NAME_VERSION;
    text = version_text.c_str();
    ui_get_string_size(&w, &h, text, -1, font_id);
    x = 430 - w;
    w += 5;
    h += 2;
    UiLabel_create2_version_label_hook.call_target(self, edx, parent, x, y, w, h, text, font_id);
}
CallHook<UiLabel_Create2_Type> UiLabel_create2_version_label_hook{0x0044344D, UiLabel_create2_version_label};

CallHook<void()> main_menu_process_mouse_hook{
    0x004437B9,
    []() {
        main_menu_process_mouse_hook.call_target();
        if (rf::mouse_was_button_pressed(0)) {
            int x, y, z;
            rf::mouse_get_pos(x, y, z);
            rf::ui::Gadget* gadgets_to_check[1] = {&rf::menu_version_label};
            int matched = rf::ui::get_gadget_from_pos(x, y, gadgets_to_check, std::size(gadgets_to_check));
            if (matched == 0) {
                xlog::trace("Version clicked");
                ++g_version_click_counter;
                if (g_version_click_counter == 3)
                    g_egg_anim_start = GetTickCount();
            }
        }
    },
};

int load_easter_egg_image()
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

    int hbm = rf::bm::create(rf::bm::FORMAT_8888_ARGB, easter_egg_size, easter_egg_size);

    rf::gr::LockInfo lock;
    if (!rf::gr::lock(hbm, 0, &lock, rf::gr::LOCK_WRITE_ONLY))
        return -1;

    rf::bm::convert_format(lock.data, lock.format, res_data, rf::bm::FORMAT_8888_ARGB,
                        easter_egg_size * easter_egg_size);
    rf::gr::unlock(&lock);

    return hbm;
}

CallHook<void()> main_menu_render_hook{
    0x00443802,
    []() {
        main_menu_render_hook.call_target();
        if (g_version_click_counter >= 3) {
            static int img = load_easter_egg_image(); // data.vpp
            if (img == -1)
                return;
            int w, h;
            rf::bm::get_dimensions(img, &w, &h);
            int anim_delta_time = GetTickCount() - g_egg_anim_start;
            int pos_x = (rf::gr::screen_width() - w) / 2;
            int pos_y = rf::gr::screen_height() - h;
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
            rf::gr::bitmap(img, pos_x, pos_y, rf::gr::bitmap_clamp_mode);
        }
    },
};

struct ServerListEntry
{
    char name[32];
    char level_name[32];
    char mod_name[16];
    int game_type;
    rf::NetAddr addr;
    char current_players;
    char max_players;
    int16_t ping;
    int field_60;
    char field_64;
    int flags;
};
static_assert(sizeof(ServerListEntry) == 0x6C, "invalid size");

FunHook<int(const int&, const int&)> server_list_cmp_func_hook{
    0x0044A6D0,
    [](const int& index1, const int& index2) {
        auto* server_list = addr_as_ref<ServerListEntry*>(0x0063F62C);
        bool has_ping1 = server_list[index1].ping >= 0;
        bool has_ping2 = server_list[index2].ping >= 0;
        if (has_ping1 != has_ping2) {
            return has_ping1 ? -1 : 1;
        }
        return server_list_cmp_func_hook.call_target(index1, index2);
    },
};

CodeInjection menu_draw_background_injection{
    0x00442D5C,
    [](auto& regs) {
        auto& menu_background_bitmap = addr_as_ref<int>(0x00598FEC);
        auto& menu_background_x = addr_as_ref<float>(0x0063C074);

        rf::gr::set_color(255, 255, 255, 255);
        // Use function that accepts float sx
        //for (int i = 0; i < 100; ++i)
        gr_bitmap_scaled_float(menu_background_bitmap, 0.0f, 0.0f,
            static_cast<float>(rf::gr::screen.max_w), static_cast<float>(rf::gr::screen.max_h),
            menu_background_x, 0.0f, 640.0f, 480.0f, false, false, rf::gr::bitmap_clamp_mode);
        regs.eip = 0x00442D94;
    },
};

CodeInjection multi_servers_on_list_click_injection{
    0x0044B084,
    [](auto& regs) {
        // Edi register contains mouse click X position relative to the server list box left edge.
        // It is compared to hard-coded range of coordinates that designate area used by "fav" column.
        // Those hard-coded coordinates make sense only in 640x480 resolution (menu native resolution).
        // Because of that mouse click coordinate must be scaled from screen resolution to UI resolution before
        // comparision.
        int rel_y = regs.edi;
        rel_y = static_cast<int>(static_cast<float>(rel_y) / rf::ui::scale_x);
        regs.edi = rel_y;
    },
};

CodeInjection main_menu_set_music_injection{
    0x0044323C,
    []() {
        g_game_music_sig_to_restore = rf::snd_music_sig;
    },
};

FunHook<void()> main_menu_stop_music_hook{
    0x00443E90,
    []() {
        if (rf::snd_music_sig == rf::main_menu_music_sig) {
            main_menu_stop_music_hook.call_target();
            // Restore old sig in music sig variable so game can keep track of it (and stop it when needed)
            rf::snd_music_sig = g_game_music_sig_to_restore;
        }
        else if (g_game_music_sig_to_restore != -1) {
            // Music changed when the menu was open - stop the old music
            // Note: In Single Player RF pauses the world when entering the menu so it should not happen
            //       In Multi Player on the other hand level scripts are still executing in the background and nothing stops
            //       them from playing new sounds/music. Would be cool to fix it one day...
            rf::snd_pc_stop(g_game_music_sig_to_restore);
            g_game_music_sig_to_restore = -1;
        }
    },
};

void apply_main_menu_patches()
{
    // Version in Main Menu
    UiLabel_create2_version_label_hook.install();

    // Version Easter Egg
    main_menu_process_mouse_hook.install();
    main_menu_render_hook.install();

    // Put not responding servers at the bottom of server list
    server_list_cmp_func_hook.install();

    // Fix multi menu having background scroll speed doubled
    write_mem<int8_t>(0x00443C2E + 1, 0);
    write_mem<int8_t>(0x00443C30 + 1, 0);

    // Make menu background scrolling smooth on high resolutions
    menu_draw_background_injection.install();

    // Fix clicking fav checkbox in server list
    multi_servers_on_list_click_injection.install();

    // Fix music being unstoppable after opening the menu
    main_menu_set_music_injection.install();
    main_menu_stop_music_hook.install();
}
