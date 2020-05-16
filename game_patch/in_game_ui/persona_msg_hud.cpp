#include "hud_internal.h"
#include "../rf/hud.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/game_seq.h"
#include "../rf/sound.h"
#include <patch_common/AsmWriter.h>
#include <algorithm>

bool g_big_hud_persona = false;

#define HUD_PERSONA_TEST 0

void HudRenderPersonaMsg(rf::Player* player)
{
#if HUD_PERSONA_TEST
    rf::hud_persona_alpha = 1.0f;
    rf::hud_persona_current_idx = 0;
    rf::hud_personas_tbl[rf::hud_persona_current_idx].image_bmh =
        rf::BmLoad(rf::hud_personas_tbl[rf::hud_persona_current_idx].image.CStr(), -1, false);
    std::strcpy(rf::hud_personas_tbl[rf::hud_persona_current_idx].message, "Test message! Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.");
#endif

    if (rf::hud_persona_alpha <= rf::hud_persona_target_alpha) {
        rf::hud_persona_alpha = rf::hud_persona_target_alpha;
    }
    else {
        rf::hud_persona_alpha = std::max(rf::hud_persona_alpha - rf::frametime * 1.3333334f, rf::hud_persona_target_alpha);
    }
    if (rf::hud_persona_alpha <= 0.0) {
        rf::hud_persona_current_idx = -1;
    }
    if (rf::hud_persona_current_idx == -1 || !rf::GameSeqIsCurrentlyInGame()) {
        return;
    }
    auto entity = rf::EntityGetByHandle(player->entity_handle);
    if (!entity) {
        return;
    }

    int offset_y = 0;
    auto parent = rf::EntityGetByHandle(entity->parent_handle);
    if (parent && (rf::EntityIsSub(parent) || rf::EntityIsDriller(parent) || rf::EntityIsJeep(parent) || rf::EntityIsFighter(parent)))
        offset_y = rf::hud_points[rf::hud_persona_sub_offset].y; // 360
    auto& hud_persona = rf::hud_personas_tbl[rf::hud_persona_current_idx];
    if (hud_persona.fully_visible_timer.IsFinished() && !rf::IsSoundPlaying(hud_persona.sound)) {
        rf::hud_persona_target_alpha = 0.0f;
    }
    int box_border = 2;
    int clip_w = rf::GrGetClipWidth();
    int box_w = clip_w - (g_big_hud_persona ? 650 : 313);
    int box_h = (g_big_hud_persona ? 165 : 106);
    int box_x = (clip_w - box_w) / 2;
    int box_y = offset_y + rf::hud_points[rf::hud_persona_message_box_background_ul].y; // 10
    int content_w = box_w - 2 * box_border;
    int content_h = box_h - 2 * box_border;
    int hud_persona_font = g_big_hud_persona ? rf::rfpc_large_font_id : rf::rfpc_medium_font_id;

    // border
    rf::GrSetColor(255, 255, 255, static_cast<int>(rf::hud_persona_alpha * 77.0f));
    rf::GrRect(box_x, box_y, box_border, box_h); // left
    rf::GrRect(box_x + box_w - box_border, box_y, box_border, box_h); // right
    rf::GrRect(box_x + box_border, box_y, content_w, box_border); // top
    rf::GrRect(box_x + box_border, offset_y + box_h + 8, content_w, box_border); // bottom

    // background
    rf::GrSetColor(0, 0, 0, static_cast<int>(rf::hud_persona_alpha * 128.0f));
    rf::GrRect(box_x + box_border, box_y + box_border, content_w, content_h);

    // persona image background (black)
    float img_scale = g_big_hud_persona ? 2.0f : 1.0f;
    int img_w = 64 * img_scale;
    int img_h = 64 * img_scale;
    int img_box_x = box_x + box_border; // 159
    int img_box_y = box_y + box_border; // 12
    int img_border = 2;
    rf::GrSetColorPtr(&rf::hud_msg_bg_color);
    rf::GrSetColorAlpha(static_cast<int>(rf::hud_msg_bg_color.alpha * rf::hud_persona_alpha));
    rf::GrRect(img_box_x, img_box_y, img_w + 2 * img_border, img_h + 2 * img_border);

    // persona image (64x64)
    //rf::GrSetColorPtr(&hud_default_color);
    //rf::GrSetColorAlpha(static_cast<int>(hud_default_color.alpha * hud_persona_alpha));
    rf::GrSetColor(255, 255, 255, static_cast<int>(rf::hud_default_color.alpha * rf::hud_persona_alpha));
    int img_x = img_box_x + img_border; // 161
    int img_y = img_box_y + img_border; // 14
    HudScaledBitmap(hud_persona.image_bmh, img_x, img_y, img_scale);// hud_persona_image_render_state

    // persona name
    rf::GrSetColorPtr(&rf::hud_msg_color);
    rf::GrSetColorAlpha(static_cast<int>(rf::hud_msg_color.alpha * rf::hud_persona_alpha));
    const char* display_name;
    if (hud_persona.display_name.Size() == 0) {
        display_name = hud_persona.name.CStr();
    }
    else {
        display_name = hud_persona.display_name.CStr();
    }
    int img_center_x = img_x + img_w / 2;
    int img_title_y = img_y + img_h + img_border + 2;
    rf::GrStringAligned(rf::GR_ALIGN_CENTER, img_center_x, img_title_y, display_name, hud_persona_font);

    // message
    int text_x = img_x + img_w + img_border + (g_big_hud_persona ? 12 : 8);
    int text_y = box_y + 6;
    int font_h = rf::GrGetFontHeight(hud_persona_font);
    int max_text_x = box_x + box_w - box_border - 8;
    int max_text_w = max_text_x - text_x; // rf::hud_points[hud_persona_message_box_text_area_width].x
    constexpr int max_lines = 8;
    int len_array[max_lines], offset_array[max_lines];
    int num_lines = rf::GrFitMultilineText(len_array, offset_array, hud_persona.message,
        max_text_w, max_lines, 0, hud_persona_font);
    rf::GrSetColorPtr(&rf::hud_default_color);
    rf::GrSetColorAlpha(static_cast<int>(rf::hud_default_color.alpha * rf::hud_persona_alpha));
    for (int i = 0; i < num_lines; ++i) {
        char buf[256];
        std::strncpy(buf, &hud_persona.message[offset_array[i]], len_array[i]);
        buf[len_array[i]] = 0;
        rf::GrString(text_x, text_y, buf, hud_persona_font);
        text_y += font_h;
    }
}

void InstallPersonaMsgHudPatches()
{
    AsmWriter{0x00439610}.jmp(HudRenderPersonaMsg);
}

void SetBigPersonaMsgHud(bool is_big)
{
    g_big_hud_persona = is_big;
}
