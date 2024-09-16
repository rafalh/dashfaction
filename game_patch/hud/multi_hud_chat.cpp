#include <patch_common/FunHook.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include "../rf/hud.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/multi.h"
#include "../rf/player/player.h"
#include "../hud/hud.h"
#include "../os/console.h"
#include "../misc/player.h"
#include "hud_internal.h"

bool g_big_chatbox = false;
bool g_all_players_muted = false;

constexpr int chat_msg_max_len = 224;
constexpr int chatbox_border_alpha = 0x30; // default is 77
constexpr int chatbox_bg_alpha = 0x40; // default is 128

FunHook<void(uint16_t)> multi_chat_say_add_char_hook{
    0x00444740,
    [](uint16_t key) {
        if (key)
            multi_chat_say_add_char_hook.call_target(key);
    },
};

FunHook<void(const char*, bool)> multi_chat_say_accept_hook{
    0x00444440,
    [](const char* msg, bool is_team_msg) {
        std::string msg_str{msg};
        if (msg_str.size() > chat_msg_max_len)
            msg_str.resize(chat_msg_max_len);
        multi_chat_say_accept_hook.call_target(msg_str.c_str(), is_team_msg);
    },
};

void multi_hud_render_chat()
{
    if (!rf::chat_fully_visible_timer.valid() && !rf::chat_fade_out_timer.valid()) {
        return;
    }

    float fade_out = 1.0;
    if (rf::chat_fully_visible_timer.elapsed() && !rf::chat_fade_out_timer.valid()) {
        rf::chat_fully_visible_timer.invalidate();
        rf::chat_fade_out_timer.set(750);
    }
    if (rf::chat_fade_out_timer.valid()) {
        if (rf::chat_fade_out_timer.elapsed()) {
            rf::chat_fade_out_timer.invalidate();
            return;
        }
        fade_out = rf::chat_fade_out_timer.time_until() / 750.0f;
    }

    int chatbox_font = hud_get_default_font();
    int clip_w = rf::gr::clip_width();
    int font_h = rf::gr::get_font_height(chatbox_font);
    int border = g_big_chatbox ? 3 : 2;
    int box_w = clip_w - (g_big_chatbox ? 600 : 313);
    int box_h = 8 * font_h + 2 * border + 6;
    int content_w = box_w - 2 * border;
    int content_h = box_h - 2 * border;
    int box_y = 10;
    int box_x = (clip_w - box_w) / 2;

    int border_alpha = rf::scoreboard_visible ? 255 : chatbox_border_alpha;
    rf::gr::set_color(255, 255, 255, static_cast<int>(border_alpha * fade_out));
    hud_rect_border(box_x, box_y, box_w, box_h, border);
    int bg_alpha = rf::scoreboard_visible ? 255 : chatbox_bg_alpha;
    rf::gr::set_color(0, 0, 0, static_cast<int>(bg_alpha * fade_out));
    rf::gr::rect(box_x + border, box_y + border, content_w, content_h);
    int y = box_y + box_h - border - font_h - 5;

    int text_alpha = static_cast<int>(fade_out * 255.0);
    for (auto& msg : rf::chat_messages) {
        if (msg.name.empty() && msg.text.empty()) {
            break;
        }
        int x = box_x + border + 6;

        int name_w, name_h;
        rf::gr::get_string_size(&name_w, &name_h, msg.name.c_str(), -1, chatbox_font);
        if (msg.color_id == 0 || msg.color_id == 1) {
            if (msg.color_id == 0) {
                rf::gr::set_color(227, 48, 47, text_alpha);
            }
            else {
                rf::gr::set_color(117, 117, 254, text_alpha);
            }
            rf::gr::string(x, y, msg.name.c_str(), chatbox_font);
            rf::gr::set_color(255, 255, 255, text_alpha);
            x += name_w;
        }
        else if (msg.color_id == 2) {
            rf::gr::set_color(227, 48, 47, text_alpha);
        }
        else if (msg.color_id == 3) {
            rf::gr::set_color(117, 117, 254, text_alpha);
        }
        else if (msg.color_id == 4) {
            rf::gr::set_color(255, 255, 255, text_alpha);
        }
        else {
            rf::gr::set_color(52, 255, 57, text_alpha);
        }
        rf::gr::string(x, y, msg.text.c_str(), chatbox_font);
        y -= font_h;
    }
}

FunHook<void()> multi_hud_render_chat_hook{0x004773D0, multi_hud_render_chat};

CodeInjection multi_hud_add_chat_line_max_width_injection{
    0x004788E3,
    [](auto& regs) {
        regs.esi = rf::gr::screen_width() - (g_big_chatbox ? 620 : 320);
    },
};

void multi_hud_render_chat_inputbox(rf::String::Pod label_pod, rf::String::Pod msg_pod)
{
    // Note: POD has to be used here because of differences between compilers ABI
    rf::String label{label_pod};
    rf::String msg{msg_pod};
    if (label.empty() && msg.empty()) {
        return;
    }

    int chatbox_font = hud_get_default_font();
    int clip_w = rf::gr::clip_width();
    int font_h = rf::gr::get_font_height(chatbox_font); // 12
    int border = g_big_chatbox ? 3 : 2;
    int box_w = clip_w - (g_big_chatbox ? 600 : 313);
    int content_w = box_w - 2 * border; // clip_w - 317
    int hist_box_y = 10;
    int hist_box_h = 8 * font_h + 2 * border + 6;
    int input_box_content_h = font_h + 3;
    int input_box_h = input_box_content_h + 2 * border;
    int input_box_y = hist_box_y + hist_box_h; // 116
    int input_box_x = (clip_w - box_w) / 2; // 157

    rf::String msg_shortened{msg};
    int msg_w, msg_h;
    rf::gr::get_string_size(&msg_w, &msg_h, msg_shortened.c_str(), -1, chatbox_font);
    int label_w, label_h;
    rf::gr::get_string_size(&label_w, &label_h, label.c_str(), -1, chatbox_font);
    int cursor_w = g_big_chatbox ? 10 : 5;
    int cursor_h = g_big_chatbox ? 2 : 1;
    int max_msg_w = content_w - cursor_w - 7 - label_w;
    if (max_msg_w <= 0) {
        // Can happen with big HUD + very low resolution
        return;
    }
    while (msg_w > max_msg_w) {
        msg_shortened = msg_shortened.substr(1, -1);
        rf::gr::get_string_size(&msg_w, &msg_h, msg_shortened.c_str(), -1, chatbox_font);
    }

    rf::gr::set_color(255, 255, 255, rf::scoreboard_visible ? 255 : chatbox_border_alpha);
    hud_rect_border(input_box_x, input_box_y, box_w, input_box_h, 2);

    rf::gr::set_color(0, 0, 0, rf::scoreboard_visible ? 255 : chatbox_bg_alpha);
    rf::gr::rect(input_box_x + border, input_box_y + border, content_w, input_box_content_h);

    rf::String label_and_msg = rf::String::concat(label, msg_shortened);
    int text_x = input_box_x + border + 4;
    int text_y = input_box_y + border + (g_big_chatbox ? 2 : 1);
    rf::gr::set_color(53, 207, 22, 255);
    rf::gr::string(text_x, text_y, label_and_msg.c_str(), chatbox_font);

    static rf::Timestamp cursor_blink_timestamp;
    static bool chatbox_cursor_visible = false;

    if (!cursor_blink_timestamp.valid() || cursor_blink_timestamp.elapsed()) {
        cursor_blink_timestamp.set(350);
        chatbox_cursor_visible = !chatbox_cursor_visible;
    }
    if (chatbox_cursor_visible) {
        int cursor_x = text_x + label_w + msg_w + 2;
        int cursor_y = text_y + msg_h + (g_big_chatbox ? -2 : 0);

        rf::gr::rect(cursor_x, cursor_y, cursor_w, cursor_h);
    }
}

FunHook<void(rf::String::Pod, rf::String::Pod)> multi_hud_render_chat_inputbox_hook{0x00478CA0, multi_hud_render_chat_inputbox};

static bool is_muted_player(rf::Player *pp)
{
    if (g_all_players_muted) {
        return true;
    }
    auto& pdata = get_player_additional_data(pp);
    return pdata.is_muted;
}

CodeInjection process_chat_line_packet_injection{
    0x00444948,
    [](auto& regs) {
        rf::Player* pp = regs.eax;
        const char* message = regs.ecx;
        int mode = regs.edx;
        if (is_muted_player(pp)) {
            // Messages from muted players only show up in the console
            const char *team_str = mode == 1 ? rf::strings::team : "";
            rf::console::print("{}{} (MUTED): {}", pp->name, team_str, message);
            regs.eip = 0x00444958;
        }
    },
};

ConsoleCommand2 mute_all_players_cmd{
    "mute_all_players",
    []() {
        g_all_players_muted = !g_all_players_muted;
        rf::console::print("All players are {}", g_all_players_muted ? "muted" : "unmuted");
    },
    "Mutes all players in multiplayer chat",
};

ConsoleCommand2 mute_player_cmd{
    "mute_player",
    [](std::string player_name) {
        rf::Player* pp = find_best_matching_player(player_name.c_str());
        if (pp) {
            auto& pdata = get_player_additional_data(pp);
            pdata.is_muted = !pdata.is_muted;
            rf::console::print("Player {} is {}", pp->name, pdata.is_muted ? "muted" : "unmuted");
        }
    },
    "Mutes a single player in multiplayer chat",
};

void multi_hud_chat_apply_patches()
{
    // Fix game beeping every frame if chat input buffer is full
    multi_chat_say_add_char_hook.install();

    // Change chat input limit to 224 (RF can support 255 safely but PF kicks if message is longer than 224)
    write_mem<i32>(0x0044474A + 1, chat_msg_max_len);

    // Add chat message limit for say/teamsay commands
    multi_chat_say_accept_hook.install();

    // Big HUD support
    multi_hud_render_chat_hook.install();
    multi_hud_add_chat_line_max_width_injection.install();
    multi_hud_render_chat_inputbox_hook.install();

    // Muting support
    process_chat_line_packet_injection.install();
    mute_all_players_cmd.register_cmd();
    mute_player_cmd.register_cmd();

    // Do not strip '%' characters from chat messages
    write_mem<u8>(0x004785FD, asm_opcodes::jmp_rel_short);
}

void multi_hud_chat_set_big(bool is_big)
{
    g_big_chatbox = is_big;
}
