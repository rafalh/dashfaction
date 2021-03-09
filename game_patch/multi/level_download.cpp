#include <common/HttpRequest.h>
#include <common/rfproto.h>
#include <common/config/BuildConfig.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <xlog/xlog.h>
#include <stdexcept>
#include <thread>
#include <future>
#include <unrar/dll.hpp>
#include <unzip.h>
#include <windows.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include "../rf/multi.h"
#include "../rf/file/file.h"
#include "../rf/file/packfile.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/ui.h"
#include "../misc/misc.h"
#include "../os/console.h"
#include "multi.h"

struct LevelDownloadInfo
{
    std::string name;
    std::string author;
    std::string description;
    unsigned size_in_bytes;
    int ticket_id;
};

static const char level_download_agent_name[] = "Dash Faction";
static const char level_download_base_url[] = "http://pfapi.factionfiles.com";

static LevelDownloadInfo g_level_info;
static std::atomic<unsigned> g_level_bytes_downloaded;
static std::shared_ptr<std::atomic<bool>> g_level_download_cancel_flag_ptr;
static std::atomic<float> g_level_download_bytes_per_sec;
static std::function<void(bool)> g_level_download_callback;
bool g_level_download_show_popup;
static std::future<std::optional<LevelDownloadInfo>> g_level_info_future;
static std::future<std::string> g_level_download_future;
static std::future<std::vector<std::string>> g_level_unpack_future;

enum class LevelDownloadState
{
    idle,
    fetching_info,
    fetching_data,
    uncompressing,
} g_level_download_state = LevelDownloadState::idle;

static bool is_vpp_filename(const char* filename)
{
    return string_ends_with_ignore_case(filename, ".vpp");
}

static std::vector<std::string> unzip_vpp(const char* path)
{
    unzFile archive = unzOpen(path);
    if (!archive) {
#ifdef DEBUG
        xlog::error("unzOpen failed: %s", path);
#endif
        return {};
    }

    unz_global_info global_info;
    int code = unzGetGlobalInfo(archive, &global_info);
    if (code != UNZ_OK) {
        xlog::error("unzGetGlobalInfo failed - error %d, path %s", code, path);
        return {};
    }

    std::vector<std::string> packfiles;
    char buf[4096], file_name[MAX_PATH];
    unz_file_info file_info;
    for (unsigned long i = 0; i < global_info.number_entry; i++) {
        code = unzGetCurrentFileInfo(archive, &file_info, file_name, sizeof(file_name), nullptr, 0, nullptr, 0);
        if (code != UNZ_OK) {
            xlog::error("unzGetCurrentFileInfo failed - error %d, path %s", code, path);
            break;
        }

        if (is_vpp_filename(file_name)) {
#ifdef DEBUG
            xlog::trace("Unpacking %s", file_name);
#endif
            auto output_path = string_format("%suser_maps\\multi\\%s", rf::root_path, file_name);
            std::ofstream file(output_path, std::ios_base::out | std::ios_base::binary);
            if (!file) {
                xlog::error("Cannot open file: %s", output_path.c_str());
                break;
            }

            code = unzOpenCurrentFile(archive);
            if (code != UNZ_OK) {
                xlog::error("unzOpenCurrentFile failed - error %d, path %s", code, path);
                break;
            }

            while ((code = unzReadCurrentFile(archive, buf, sizeof(buf))) > 0) file.write(buf, code);

            if (code < 0) {
                xlog::error("unzReadCurrentFile failed - error %d, path %s", code, path);
                break;
            }

            file.close();
            unzCloseCurrentFile(archive);

            packfiles.push_back(file_name);
        }

        if (i + 1 < global_info.number_entry) {
            code = unzGoToNextFile(archive);
            if (code != UNZ_OK) {
                xlog::error("unzGoToNextFile failed - error %d, path %s", code, path);
                break;
            }
        }
    }

    unzClose(archive);
    xlog::debug("Unzipped");
    return packfiles;
}

static std::vector<std::string> unrar_vpp(const char* path)
{
    char cmt_buf[16384];

    RAROpenArchiveDataEx open_archive_data;
    memset(&open_archive_data, 0, sizeof(open_archive_data));
    open_archive_data.ArcName = const_cast<char*>(path);
    open_archive_data.CmtBuf = cmt_buf;
    open_archive_data.CmtBufSize = sizeof(cmt_buf);
    open_archive_data.OpenMode = RAR_OM_EXTRACT;
    open_archive_data.Callback = nullptr;
    HANDLE archive_handle = RAROpenArchiveEx(&open_archive_data);

    if (!archive_handle || open_archive_data.OpenResult != 0) {
        xlog::error("RAROpenArchiveEx failed - result %d, path %s", open_archive_data.OpenResult, path);
        return {};
    }

    std::vector<std::string> packfiles;
    struct RARHeaderData header_data;
    header_data.CmtBuf = nullptr;
    memset(&open_archive_data.Reserved, 0, sizeof(open_archive_data.Reserved));

    while (true) {
        int code = RARReadHeader(archive_handle, &header_data);
        if (code == ERAR_END_ARCHIVE)
            break;

        if (code != 0) {
            xlog::error("RARReadHeader failed - result %d, path %s", code, path);
            break;
        }

        if (is_vpp_filename(header_data.FileName)) {
            xlog::trace("Unpacking %s", header_data.FileName);
            auto output_dir = string_format("%suser_maps\\multi", rf::root_path);
            code = RARProcessFile(archive_handle, RAR_EXTRACT, const_cast<char*>(output_dir.c_str()), nullptr);
            if (code == 0) {
                packfiles.push_back(header_data.FileName);
            }
        }
        else {
            xlog::trace("Skipping %s", header_data.FileName);
            code = RARProcessFile(archive_handle, RAR_SKIP, nullptr, nullptr);
        }

        if (code != 0) {
            xlog::error("RARProcessFile failed - result %d, path %s", code, path);
            break;
        }
    }

    RARCloseArchive(archive_handle);
    xlog::debug("Unrared");
    return packfiles;
}

static void fetch_level_file(const char* tmp_filename, int ticket_id, std::atomic<bool>& cancel_flag)
{
    HttpSession session{level_download_agent_name};
    auto url = string_format("%s/downloadmap.php?ticketid=%u", level_download_base_url, ticket_id);
    HttpRequest req{url, "GET", session};
    req.send();

    std::ofstream tmp_file(tmp_filename, std::ios_base::out | std::ios_base::binary);
    if (!tmp_file) {
        xlog::error("Cannot open file: %s", tmp_filename);
        throw std::runtime_error("cannot open file");
    }

    tmp_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    auto download_start = std::chrono::steady_clock::now();

    char buf[4096];
    while (true) {
        auto num_bytes_read = req.read(buf, sizeof(buf));
        if (num_bytes_read <= 0)
            break;

        tmp_file.write(buf, num_bytes_read);

        g_level_bytes_downloaded += num_bytes_read;
        auto time_diff = std::chrono::steady_clock::now() - download_start;
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff).count();
        if (duration_ms > 0) {
            g_level_download_bytes_per_sec = g_level_bytes_downloaded * 1000.0f / duration_ms;
        }

        if (cancel_flag) {
            xlog::debug("Download canceled");
            throw std::runtime_error("canceled");
        }
    }
}

static std::string get_temp_filename_in_temp_dir(const char* prefix)
{
    char temp_dir[MAX_PATH];
    DWORD ret_val = GetTempPathA(std::size(temp_dir), temp_dir);
    if (ret_val == 0 || ret_val > std::size(temp_dir)) {
        throw std::runtime_error("GetTempPathA failed");
    }

    char result[MAX_PATH];
    ret_val = GetTempFileNameA(temp_dir, prefix, 0, result);
    if (ret_val == 0) {
        throw std::runtime_error("GetTempFileNameA failed");
    }

    return result;
}

static std::string download_level_file_by_id(int ticket_id, std::shared_ptr<std::atomic<bool>> cancel_flag_ptr)
{
    auto temp_filename = get_temp_filename_in_temp_dir("DF_Level_");

    try {
        fetch_level_file(temp_filename.c_str(), ticket_id, *cancel_flag_ptr);
    }
    catch (const std::exception& ex) {
        remove(temp_filename.c_str());
        throw ex;
    }
    return temp_filename;
}

static std::optional<LevelDownloadInfo> parse_level_download_info(const char* buf)
{
    std::stringstream ss(buf);
    ss.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string temp;
    std::getline(ss, temp);
    if (temp != "found") {
        // not found
        return {};
    }

    LevelDownloadInfo info;
    std::getline(ss, info.name);
    std::getline(ss, info.author);
    std::getline(ss, info.description);

    std::getline(ss, temp);
    info.size_in_bytes = static_cast<unsigned>(std::stof(temp) * 1024u * 1024u);
    if (!info.size_in_bytes) {
        throw std::runtime_error("invalid file size");
    }

    std::getline(ss, temp);
    info.ticket_id = std::stoi(temp);
    if (info.ticket_id <= 0) {
        throw std::runtime_error("invalid ticket id");
    }

    return {info};
}

static std::optional<LevelDownloadInfo> fetch_level_download_info(std::string file_name)
{
    HttpSession session{level_download_agent_name};
    session.set_connect_timeout(2000);
    session.set_receive_timeout(3000);
    auto url = std::string{level_download_base_url} + "/findmap.php";

    xlog::trace("Fetching level info: %s", file_name.c_str());
    HttpRequest req{url, "POST", session};
    std::string body = std::string("rflName=") + file_name;

    req.set_content_type("application/x-www-form-urlencoded");
    req.send(body);

    char buf[256];
    size_t num_bytes_read = req.read(buf, sizeof(buf) - 1);
    if (num_bytes_read == 0) {
        throw std::runtime_error("empty response");
    }

    buf[num_bytes_read] = '\0';
    xlog::debug("FactionFiles response: %s", buf);

    return parse_level_download_info(buf);
}

void level_download_cancel()
{
    rf::ui_popup_abort();
    if (g_level_download_callback) {
        //g_level_download_callback(false);
        g_level_download_callback = {};
    }
    if (g_level_download_cancel_flag_ptr) {
        *g_level_download_cancel_flag_ptr = true;
        g_level_download_cancel_flag_ptr = {};
    }
    g_level_download_state = LevelDownloadState::idle;
}

static std::string build_download_popup_text()
{
    return string_format(
        "Level name: %s\nAuthor: %s\n%.2f MB / %.2f MB (%.2f MB/s)",
        g_level_info.name.c_str(),
        g_level_info.author.c_str(),
        g_level_bytes_downloaded / 1024.0f / 1024.0f,
        g_level_info.size_in_bytes / 1024.0f / 1024.0f,
        g_level_download_bytes_per_sec / 1024.0f / 1024.0f
    );
}

static void display_download_popup(LevelDownloadInfo& level_info)
{
    xlog::debug("Download ticket id: %u", level_info.ticket_id);
    g_level_info = level_info;

    if (g_level_download_show_popup) {
        auto text = build_download_popup_text();
        const char* choices[] = {"Cancel"};
        rf::UiDialogCallbackPtr callbacks[] = {level_download_cancel};
        rf::ui_popup_custom("Downloading level...", text.c_str(), std::size(choices), choices, callbacks, 0, nullptr);
    }
}

bool try_to_download_level(const char* filename, bool popup, std::function<void(bool)> callback)
{
    if (g_level_download_state != LevelDownloadState::idle) {
        xlog::error("Level download already in progress!");
        return false;
    }

    g_level_download_callback = callback;
    g_level_download_cancel_flag_ptr = std::make_shared<std::atomic<bool>>(false);
    g_level_download_show_popup = popup;

    if (g_level_download_show_popup) {
        auto text = "Searching for the level in FactionFiles.com database...";
        const char* choices[] = {"Cancel"};
        rf::UiDialogCallbackPtr callbacks[] = {level_download_cancel};
        rf::ui_popup_custom("Level Download", text, std::size(choices), choices, callbacks, 0, nullptr);
    }

    xlog::trace("Fetching level info");
    std::string filename_owned = filename;
    g_level_info_future = std::async(std::launch::async, fetch_level_download_info, filename_owned);
    g_level_download_state = LevelDownloadState::fetching_info;
    return true;
}

void level_download_fetching_info_do_frame()
{
    using namespace std::chrono_literals;
    auto status = g_level_info_future.wait_for(0ms);
    if (status != std::future_status::ready) {
        return;
    }

    std::optional<LevelDownloadInfo> level_info_opt;
    try {
        level_info_opt = g_level_info_future.get();
    }
    catch (const std::exception& e) {
        xlog::error("Failed to fetch level info: %s", e.what());
    }

    rf::ui_popup_abort();
    if (!level_info_opt) {
        level_download_cancel();
        rf::ui_popup_message("Level Download", "Level cannot be found", nullptr, false);
        return;
    }

    g_level_bytes_downloaded = 0;
    g_level_download_state = LevelDownloadState::fetching_data;
    auto level_info = level_info_opt.value();
    display_download_popup(level_info);
    g_level_download_future = std::async(download_level_file_by_id, level_info.ticket_id, g_level_download_cancel_flag_ptr);
}

std::vector<std::string> level_download_do_uncompress(std::string temp_filename)
{
    try {
        std::vector<std::string> packfiles = unzip_vpp(temp_filename.c_str());
        if (packfiles.empty()) {
            packfiles = unrar_vpp(temp_filename.c_str());
        }
        if (packfiles.empty()) {
            xlog::error("unzip_vpp and unrar_vpp failed");
        }
        remove(temp_filename.c_str());
        return packfiles;
    }
    catch (const std::exception& e) {
        remove(temp_filename.c_str());
        throw e;
    }
}

void level_download_fetching_data_do_frame()
{
    using namespace std::chrono_literals;
    auto status = g_level_download_future.wait_for(0ms);
    if (status != std::future_status::ready) {
        return;
    }

    std::string temp_filename;
    try {
        temp_filename = g_level_download_future.get();
    }
    catch (const std::exception& e) {
        xlog::error("Failed to fetch level file: %s", e.what());
        level_download_cancel();
        rf::ui_popup_message("Level Download", "Failed to download level", nullptr, false);
        return;
    }

    xlog::debug("Unpacking level from %s", temp_filename.c_str());
    g_level_unpack_future = std::async(level_download_do_uncompress, temp_filename);
    g_level_download_state = LevelDownloadState::uncompressing;

    rf::ui_popup_set_text("Uncompressing and loading packfiles...");
}

void level_download_uncompressing_do_frame()
{
    using namespace std::chrono_literals;
    auto status = g_level_unpack_future.wait_for(0ms);
    if (status != std::future_status::ready) {
        return;
    }

    std::vector<std::string> packfiles;
    try {
        packfiles = g_level_unpack_future.get();
    }
    catch (const std::exception& e) {
        xlog::error("Failed to extract packfiles: %s", e.what());
    }

    rf::vpackfile_set_loading_user_maps(true);
    for (auto& filename : packfiles) {
        if (!rf::vpackfile_add(filename.c_str(), "user_maps\\multi\\")) {
            xlog::error("vpackfile_add failed - %s", filename.c_str());
        }
    }
    rf::vpackfile_set_loading_user_maps(false);

    rf::ui_popup_abort();
    g_level_download_state = LevelDownloadState::idle;

    if (g_level_download_callback) {
        g_level_download_callback(true);
    }
}

void level_download_do_frame()
{
    switch (g_level_download_state) {
        case LevelDownloadState::fetching_info:
            level_download_fetching_info_do_frame();
            break;
        case LevelDownloadState::fetching_data:
            level_download_fetching_data_do_frame();
            break;
        case LevelDownloadState::uncompressing:
            level_download_uncompressing_do_frame();
            break;
        case LevelDownloadState::idle:
            // nothing to do
            break;
    }
}

void render_progress_bar(int x, int y, int w, int h, float progress)
{
    int border = 2;
    int inner_w = w - 2 * border;
    int inner_h = h - 2 * border;
    int progress_w = static_cast<int>(static_cast<float>(inner_w) * progress);
    if (progress_w > inner_w) {
        progress_w = inner_w;
    }

    int inner_x = x + border;
    int inner_y = y + border;

    rf::gr_set_color(0x40, 0x40, 0x40, 0xFF);
    rf::gr_rect(x, y, w, h);

    if (progress_w > 0) {
        rf::gr_set_color(0, 0x80, 0, 0xFF);
        rf::gr_rect(inner_x, inner_y, progress_w, inner_h);
    }

    if (w > progress_w) {
        rf::gr_set_color(0, 0, 0, 0xFF);
        rf::gr_rect(inner_x + progress_w, inner_y, inner_w - progress_w, inner_h);
    }
}

static void level_download_render_progress_bar(int x, int y, int w, int h)
{
    float progress = static_cast<float>(g_level_bytes_downloaded) / static_cast<float>(g_level_info.size_in_bytes);
    render_progress_bar(x, y, w, h, progress);
}

void maybe_rejoin_server_after_level_download(bool download_success, rf::NetAddr server_addr)
{
    if (download_success) {
        multi_join_game(server_addr, "");
    }
}

#include "../rf/input.h"
#include "../rf/gameseq.h"
#include "../rf/level.h"
#include "../rf/gameseq.h"
#include "../hud/hud.h"

namespace rf
{
    enum ChatSayType {
        CHAT_SAY_GLOBAL = 0,
        CHAT_SAY_TEAM = 1,
    };
    static auto& multi_chat_say_handle_key = addr_as_ref<void(int key)>(0x00444620);
    static auto& multi_chat_is_say_visible = addr_as_ref<bool()>(0x00444AC0);
    static auto& multi_chat_say_render = addr_as_ref<void()>(0x00444790);
    static auto& multi_chat_say_show = addr_as_ref<void(ChatSayType type)>(0x00444A80);
    static auto& multi_hud_render_chat = addr_as_ref<void()>(0x004773D0);
    static auto& game_poll = addr_as_ref<void(void(*key_callback)(rf::Key k))>(0x004353C0);
    static auto& scoreboard_render_internal = addr_as_ref<void(bool netgame_scoreboard)>(0x00470880);
}

void multi_level_download_handle_input(rf::Key key)
{
    if (!key) {
        return;
    }
    else if (rf::multi_chat_is_say_visible()) {
        rf::multi_chat_say_handle_key(key);
    }
    else if (key == rf::KEY_ESC) {
         rf::gameseq_push_state(rf::GS_MAIN_MENU, false, false);
    }
}

void multi_level_download_do_frame()
{
    rf::game_poll(multi_level_download_handle_input);

    int scr_w = rf::gr_screen_width();
    int scr_h = rf::gr_screen_height();

    static int bg_bm = rf::bm_load("demo-gameover.tga", -1, false);
    int bg_bm_w, bg_bm_h;
    rf::bm_get_dimensions(bg_bm, &bg_bm_w, &bg_bm_h);
    rf::gr_set_color(255, 255, 255, 255);
    rf::gr_bitmap_scaled(bg_bm, 0, 0, scr_w, scr_h, 0, 0, bg_bm_w, bg_bm_h);

    rf::multi_hud_render_chat();

    auto ccp = &rf::local_player->settings.controls;
    bool just_pressed;
    if (rf::control_config_check_pressed(ccp, rf::CC_ACTION_CHAT, &just_pressed)) {
        rf::multi_chat_say_show(rf::CHAT_SAY_GLOBAL);
    }

    int large_font = hud_get_large_font();
    int large_font_h = rf::gr_get_font_height(large_font);
    int medium_font = hud_get_default_font();
    int medium_font_h = rf::gr_get_font_height(medium_font);

    int center_x = scr_w / 2;
    int y = scr_h / 3;

    rf::gr_set_color(255, 255, 255, 255);
    rf::gr_string_aligned(rf::GR_ALIGN_CENTER, center_x, y, "DOWNLOADING LEVEL", large_font);
    y += large_font_h * 3;

    if (g_level_download_state == LevelDownloadState::fetching_info) {
        rf::gr_string_aligned(rf::GR_ALIGN_CENTER, center_x, y, "Getting level info...", medium_font);
    }
    else {
        auto level_name_str = std::string{"Level name: "} + g_level_info.name;
        int str_w, str_h;
        rf::gr_get_string_size(&str_w, &str_h, level_name_str.c_str(), -1, medium_font);
        int info_x = center_x - str_w / 2;
        int info_spacing = medium_font_h * 3 / 2;
        rf::gr_string(info_x, y, level_name_str.c_str(), medium_font);
        y += info_spacing;

        auto created_by_str = std::string{"Created by: "} + g_level_info.author;
        rf::gr_string(info_x, y, created_by_str.c_str(), medium_font);
        y += info_spacing;

        rf::gr_string(info_x, y, string_format("%.2f MB / %.2f MB (%.2f MB/s)",
            g_level_bytes_downloaded / 1000.0f / 1000.0f,
            g_level_info.size_in_bytes / 1000.0f / 1000.0f,
            g_level_download_bytes_per_sec / 1000.0f / 1000.0f).c_str(), medium_font);
        y += info_spacing;

        if (g_level_download_bytes_per_sec > 0) {
            int remaining_size = g_level_info.size_in_bytes - g_level_bytes_downloaded;
            int secs_left = remaining_size / g_level_download_bytes_per_sec;
            auto time_left_str = std::to_string(secs_left) + " seconds left";
            rf::gr_string(info_x, y, time_left_str.c_str(), medium_font);
        }
    }

    if (g_level_download_state == LevelDownloadState::fetching_data) {
        int w = 360 * rf::ui_scale_x;
        int h = 12 * rf::ui_scale_x;
        int x = (rf::gr_screen_width() - w) / 2;
        int y = rf::gr_screen_height() * 4 / 5;
        level_download_render_progress_bar(x, y, w, h);
    }

    // Scoreboard
    if (rf::control_config_check_pressed(ccp, rf::CC_ACTION_MP_STATS, nullptr)) {
        rf::scoreboard_render_internal(true);
    }
}

void multi_level_download_abort()
{
    level_download_cancel();
}

static bool next_level_exists()
{
    // rf::File file;
    // if (file.open(rf::level.next_level_filename) == 0) {
    //     return true;
    // }
    return false;
}

CallHook<void(rf::GameState, bool)> process_leave_limbo_packet_gameseq_set_next_state_hook{
    0x0047C24F,
    [](rf::GameState state, bool force) {
        if (!next_level_exists()) {
            level_download_cancel();
            rf::gameseq_set_state(rf::GS_MULTI_LEVEL_DOWNLOAD, false);
            try_to_download_level(rf::level.next_level_filename, false, [=](bool) {
                process_leave_limbo_packet_gameseq_set_next_state_hook.call_target(state, force);
            });
        }
        else {
            process_leave_limbo_packet_gameseq_set_next_state_hook.call_target(state, force);
        }
    },
};

CallHook<void(rf::GameState, bool)> game_new_game_gameseq_set_next_state_hook{
    0x00436959,
    [](rf::GameState state, bool force) {
        if (rf::is_multi && !rf::is_server && !next_level_exists()) {
            rf::gameseq_set_state(rf::GS_MULTI_LEVEL_DOWNLOAD, false);
            try_to_download_level(rf::level.next_level_filename, false, [=](bool) {
                game_new_game_gameseq_set_next_state_hook.call_target(state, force);
            });
        }
        else {
            game_new_game_gameseq_set_next_state_hook.call_target(state, force);
        }
    },
};

CodeInjection join_failed_injection{
    0x0047C4EC,
    []() {
        set_jump_to_multi_server_list(true);
    },
};

ConsoleCommand2 download_level_cmd{
    "download_level",
    [](std::string filename) {
        if (filename.rfind('.') == std::string::npos) {
            filename += ".rfl";
        }
        if (!try_to_download_level(filename.c_str(), true, [](bool) {})) {
            rf::console_printf("Another level is currently being downloaded!");
        }
    },
    "Downloads level from FactionFiles.com",
    "download_level <rfl_name>",
};

void level_download_do_patch()
{
    join_failed_injection.install();
    game_new_game_gameseq_set_next_state_hook.install();
    process_leave_limbo_packet_gameseq_set_next_state_hook.install();
}

void level_download_init()
{
    download_level_cmd.register_cmd();
}

void multi_render_level_download_progress()
{
    level_download_do_frame();

    if (g_level_download_state == LevelDownloadState::fetching_data && g_level_download_show_popup) {
        int w = 360 * rf::ui_scale_x;
        int h = 12 * rf::ui_scale_y;
        int x = (rf::gr_screen_width() - w) / 2;
        int y = rf::gr_screen_height() / 2 - rf::ui_scale_y * 20;
        level_download_render_progress_bar(x, y, w, h);
        auto popup_text = build_download_popup_text();
        rf::ui_popup_set_text(popup_text.c_str());
    }
}
