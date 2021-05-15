#include "hud_internal.h"
#include "hud.h"
#include "../rf/hud.h"
#include "../rf/entity.h"
#include "../rf/player/player.h"
#include "../rf/gameseq.h"
#include "../rf/sound/sound.h"
#include "../rf/gr/gr_font.h"
#include "../rf/os/frametime.h"
#include <patch_common/AsmWriter.h>
#include <algorithm>
#include <cstring>

bool g_big_hud_persona = false;

#define HUD_PERSONA_TEST 0

static bool hud_personas_is_bottom(rf::Entity* ep)
{
    rf::Entity* parent = rf::entity_from_handle(ep->host_handle);
    if (!parent) {
        return false;
    }
    return rf::entity_is_sub(parent) ||
        rf::entity_is_driller(parent) ||
        rf::entity_is_jeep(parent) ||
        rf::entity_is_fighter(parent);
}

void hud_personas_render(rf::Player* player)
{
#if HUD_PERSONA_TEST
    rf::hud_persona_alpha = 1.0f;
    rf::hud_persona_current_idx = 0;
    rf::hud_personas_info[rf::hud_persona_current_idx].image_bmh =
        rf::bm::load(rf::hud_personas_info[rf::hud_persona_current_idx].image.c_str(), -1, false);
    std::strcpy(rf::hud_personas_info[rf::hud_persona_current_idx].message, "Test message! Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.");
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
    if (rf::hud_persona_current_idx == -1 || !rf::gameseq_in_gameplay()) {
        return;
    }
    rf::Entity* entity = rf::entity_from_handle(player->entity_handle);
    if (!entity) {
        return;
    }

    int box_border = 2;
    int clip_w = rf::gr::clip_width();
    int clip_h = rf::gr::clip_height();
    int box_w = clip_w - (g_big_hud_persona ? 650 : 313);
    int box_h = (g_big_hud_persona ? 165 : 106);
    int box_x = (clip_w - box_w) / 2;
    int box_y;

    if (hud_personas_is_bottom(entity)) {
        // Original code uses hud_persona_sub_offset but it doesn't work right with Big HUD mode
        // box_y = rf::hud_coords[rf::hud_persona_message_box_background_ul].y +
        //     rf::hud_coords[rf::hud_persona_sub_offset].y; // 10 + 360
        box_y = clip_h - box_h - 4;
    }
    else {
        box_y = rf::hud_coords[rf::hud_persona_message_box_background_ul].y; // 10
    }
    auto& hud_persona = rf::hud_personas_info[rf::hud_persona_current_idx];
    if (hud_persona.fully_visible_timestamp.elapsed() && !rf::snd_is_playing(hud_persona.sound_instance)) {
        rf::hud_persona_target_alpha = 0.0f;
    }

    int content_w = box_w - 2 * box_border;
    int content_h = box_h - 2 * box_border;
    int hud_persona_font = hud_get_default_font();

    // border
    rf::gr::set_color(255, 255, 255, static_cast<int>(rf::hud_persona_alpha * 77.0f));
    rf::gr::rect(box_x, box_y, box_border, box_h); // left
    rf::gr::rect(box_x + box_w - box_border, box_y, box_border, box_h); // right
    rf::gr::rect(box_x + box_border, box_y, content_w, box_border); // top
    rf::gr::rect(box_x + box_border, box_y + box_h - box_border, content_w, box_border); // bottom

    // background
    rf::gr::set_color(0, 0, 0, static_cast<int>(rf::hud_persona_alpha * 128.0f));
    rf::gr::rect(box_x + box_border, box_y + box_border, content_w, content_h);

    // persona image background (black)
    float img_scale = g_big_hud_persona ? 2.0f : 1.0f;
    int img_w = static_cast<int>(64 * img_scale);
    int img_h = static_cast<int>(64 * img_scale);
    int img_box_x = box_x + box_border; // 159
    int img_box_y = box_y + box_border; // 12
    int img_border = 2;
    rf::gr::set_color(rf::hud_msg_bg_color);
    rf::gr::set_alpha(static_cast<int>(rf::hud_msg_bg_color.alpha * rf::hud_persona_alpha));
    rf::gr::rect(img_box_x, img_box_y, img_w + 2 * img_border, img_h + 2 * img_border);

    // persona image (64x64)
    //rf::gr::set_color(hud_default_color);
    //rf::gr::set_alpha(static_cast<int>(hud_default_color.alpha * hud_persona_alpha));
    rf::gr::set_color(255, 255, 255, static_cast<int>(rf::hud_default_color.alpha * rf::hud_persona_alpha));
    int img_x = img_box_x + img_border; // 161
    int img_y = img_box_y + img_border; // 14
    hud_scaled_bitmap(hud_persona.image_handle, img_x, img_y, img_scale);// hud_persona_image_gr_mode

    // persona name
    rf::gr::set_color(rf::hud_msg_color);
    rf::gr::set_alpha(static_cast<int>(rf::hud_msg_color.alpha * rf::hud_persona_alpha));
    const char* display_name;
    if (hud_persona.display_name.empty()) {
        display_name = hud_persona.name.c_str();
    }
    else {
        display_name = hud_persona.display_name.c_str();
    }
    int img_center_x = img_x + img_w / 2;
    int img_title_y = img_y + img_h + img_border + 2;
    rf::gr::string_aligned(rf::gr::ALIGN_CENTER, img_center_x, img_title_y, display_name, hud_persona_font);

    // message
    int text_x = img_x + img_w + img_border + (g_big_hud_persona ? 12 : 8);
    int text_y = box_y + 6;
    int font_h = rf::gr::get_font_height(hud_persona_font);
    int max_text_x = box_x + box_w - box_border - 8;
    int max_text_w = max_text_x - text_x; // rf::hud_coords[hud_persona_message_box_text_area_width].x
    constexpr int max_lines = 8;
    int len_array[max_lines];
    int offset_array[max_lines];
    int num_lines = rf::gr::split_str(len_array, offset_array, hud_persona.message,
        max_text_w, max_lines, 0, hud_persona_font);
    rf::gr::set_color(rf::hud_default_color);
    rf::gr::set_alpha(static_cast<int>(rf::hud_default_color.alpha * rf::hud_persona_alpha));
    for (int i = 0; i < num_lines; ++i) {
        char buf[256];
        std::strncpy(buf, &hud_persona.message[offset_array[i]], len_array[i]);
        buf[len_array[i]] = 0;
        rf::gr::string(text_x, text_y, buf, hud_persona_font);
        text_y += font_h;
    }
}

void hud_personas_apply_patches()
{
    AsmWriter{0x00439610}.jmp(hud_personas_render);
}

void hud_personas_set_big(bool is_big)
{
    g_big_hud_persona = is_big;
}
