#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <common/version.h>
#include "../rf/misc.h"
#include "../rf/graphics.h"
#include "../rf/input.h"
#include "../rf/network.h"
#include "../main.h"

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

void ApplyMainMenuPatches()
{
    // Version in Main Menu
    UiLabel_Create2_VersionLabel_hook.Install();

    // Version Easter Egg
    MenuMainProcessMouse_hook.Install();
    MenuMainRender_hook.Install();

    // Put not responding servers at the bottom of server list
    ServerListCmpFunc_hook.Install();
}
