#include <stdexcept>
#include <future>
#include <windows.h>
#include <unrar/dll.hpp>
#include <unzip.h>
#include <fstream>
#include <xlog/xlog.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <common/utils/os-utils.h>
#include <common/utils/string-utils.h>
#include "../rf/multi.h"
#include "../rf/file/file.h"
#include "../rf/file/packfile.h"
#include "../rf/gr/gr.h"
#include "../rf/gr/gr_font.h"
#include "../rf/ui.h"
#include "../misc/misc.h"
#include "../os/console.h"
#include "multi.h"
#include "faction_files.h"

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

static bool is_vpp_filename(const char* filename)
{
    return string_ends_with_ignore_case(filename, ".vpp");
}

static std::vector<std::string> unzip(const char* path, const char* output_dir,
    std::function<bool(const char*)> filename_filter)
{
    unzFile archive = unzOpen(path);
    if (!archive) {
#ifdef DEBUG
        xlog::error("unzOpen failed: %s", path);
#endif
        throw std::runtime_error{"cannot open zip file"};
    }

    unz_global_info global_info;
    int code = unzGetGlobalInfo(archive, &global_info);
    if (code != UNZ_OK) {
        xlog::error("unzGetGlobalInfo failed - error %d, path %s", code, path);
        throw std::runtime_error{"cannot open zip file"};
    }

    std::vector<std::string> extracted_files;
    char buf[4096], file_name[MAX_PATH];
    unz_file_info file_info;
    for (unsigned long i = 0; i < global_info.number_entry; i++) {
        code = unzGetCurrentFileInfo(archive, &file_info, file_name, sizeof(file_name), nullptr, 0, nullptr, 0);
        if (code != UNZ_OK) {
            xlog::error("unzGetCurrentFileInfo failed - error %d, path %s", code, path);
            break;
        }

        if (filename_filter(file_name)) {
            xlog::trace("Unpacking %s", file_name);
            auto output_path = string_format("%s\\%s", output_dir, file_name);
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

            extracted_files.push_back(file_name);
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
    return extracted_files;
}

static std::vector<std::string> unrar(const char* path, const char* output_dir,
    std::function<bool(const char*)> filename_filter)
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
        throw std::runtime_error{"cannot open rar file"};
    }

    std::vector<std::string> extracted_files;
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

        if (filename_filter(header_data.FileName)) {
            xlog::trace("Unpacking %s", header_data.FileName);
            code = RARProcessFile(archive_handle, RAR_EXTRACT, const_cast<char*>(output_dir), nullptr);
            if (code == 0) {
                extracted_files.push_back(header_data.FileName);
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
    return extracted_files;
}

enum class LevelDownloadState
{
    fetching_info,
    fetching_data,
    extracting,
    not_found,
    finished,
};

class LevelDownloadWorker
{
public:
    struct SharedData
    {
        std::atomic<LevelDownloadState> state{LevelDownloadState::fetching_info};
        std::atomic<unsigned> bytes_received{0};
        std::atomic<float> bytes_per_sec{0};
        std::atomic<bool> abort_flag{false};
        std::optional<FactionFilesClient::LevelInfo> level_info;
    };

    LevelDownloadWorker(std::string level_filename, std::shared_ptr<SharedData> shared_data) :
        level_filename_{level_filename}, shared_data_{shared_data}
    {}

    std::vector<std::string> operator()();

private:
    std::string level_filename_;
    std::shared_ptr<SharedData> shared_data_;

    void download_archive(int ticket_id, const char* temp_filename);
    std::vector<std::string> extract_archive(const char* temp_filename);
};

void LevelDownloadWorker::download_archive(int ticket_id, const char* temp_filename)
{
    auto callback = [&](unsigned bytes_received, std::chrono::milliseconds duration) {
        if (shared_data_->abort_flag) {
            return false;
        }
        shared_data_->bytes_received = bytes_received;
        auto duration_ms = duration.count();
        if (duration_ms > 0) {
            shared_data_->bytes_per_sec = bytes_received * 1000.0f / duration_ms;
        }
        return true;
    };
    FactionFilesClient ff_client;
    ff_client.download_map(temp_filename, ticket_id, callback);
}

std::vector<std::string> LevelDownloadWorker::extract_archive(const char* temp_filename)
{
    auto output_dir = string_format("%suser_maps\\multi", rf::root_path);
    std::vector<std::string> packfiles;
    try {
        packfiles = unzip(temp_filename, output_dir.c_str(), is_vpp_filename);
    }
    catch (const std::exception& e) {
        packfiles = unrar(temp_filename, output_dir.c_str(), is_vpp_filename);
    }

    if (packfiles.empty()) {
        xlog::error("No packfiles has been found in downloaded archive");
    }
    return packfiles;
}

std::vector<std::string> LevelDownloadWorker::operator()()
{
    xlog::trace("LevelDownloadWorker started");
    shared_data_->state = LevelDownloadState::fetching_info;
    FactionFilesClient ff_client;
    shared_data_->level_info = ff_client.find_map(level_filename_.c_str());
    if (!shared_data_->level_info) {
        xlog::warn("Level not found: %s", level_filename_.c_str());
        shared_data_->state = LevelDownloadState::not_found;
        return {};
    }
    xlog::trace("LevelDownloadWorker got level info");

    auto temp_filename = get_temp_path_name("DF_Level_");
    try {
        shared_data_->state = LevelDownloadState::fetching_data;
        download_archive(shared_data_->level_info.value().ticket_id, temp_filename.c_str());

        shared_data_->state = LevelDownloadState::extracting;
        std::vector<std::string> packfiles = extract_archive(temp_filename.c_str());
        remove(temp_filename.c_str());

        xlog::trace("LevelDownloadWorker finished");
        shared_data_->state = LevelDownloadState::finished;
        return packfiles;
    }
    catch (const std::exception& e) {
        remove(temp_filename.c_str());
        throw e;
    }
}

class LevelDownloadOperation
{
public:
    struct Listener
    {
        virtual ~Listener() {}
        virtual void on_progress([[ maybe_unused ]] LevelDownloadOperation& operation) {}
        virtual void on_finish([[ maybe_unused ]] LevelDownloadOperation& operation, [[ maybe_unused ]] bool success) {}
    };

private:
    std::shared_ptr<LevelDownloadWorker::SharedData> shared_data_;
    std::future<std::vector<std::string>> future_;
    std::unique_ptr<Listener> listener_;

public:
    LevelDownloadOperation(const char* level_filename, std::unique_ptr<Listener>&& listener) :
        listener_(std::move(listener))
    {
        shared_data_ = std::make_shared<LevelDownloadWorker::SharedData>();
        future_ = std::async(std::launch::async, LevelDownloadWorker{level_filename, shared_data_});
    }

    ~LevelDownloadOperation()
    {
        shared_data_->abort_flag = true;
    }

    LevelDownloadState get_state() const
    {
        return shared_data_->state;
    }

    const FactionFilesClient::LevelInfo& get_level_info() const
    {
        // check state before calling this method
        return shared_data_->level_info.value();
    }

    float get_bytes_per_sec() const
    {
        return shared_data_->bytes_per_sec;
    }

    unsigned get_bytes_received() const
    {
        return shared_data_->bytes_received;
    }

    bool in_progress() const
    {
        return future_.valid();
    }

    bool finished()
    {
        if (!future_.valid()) {
            return false;
        }
        using namespace std::chrono_literals;
        std::future_status status = future_.wait_for(0ms);
        if (status != std::future_status::ready) {
            return false;
        }
        return true;
    }

private:
    std::vector<std::string> get_pending_packfiles()
    {
        try {
            return future_.get();
        }
        catch (const std::exception& e) {
            xlog::error("Level download failed: %s", e.what());
            return {};
        }
    }

    void load_packfiles(const std::vector<std::string>& packfiles)
    {
        rf::vpackfile_set_loading_user_maps(true);
        for (auto& filename : packfiles) {
            if (!rf::vpackfile_add(filename.c_str(), "user_maps\\multi\\")) {
                xlog::error("vpackfile_add failed - %s", filename.c_str());
            }
        }
        rf::vpackfile_set_loading_user_maps(false);
    }

public:
    bool process()
    {
        if (in_progress() && listener_) {
            listener_->on_progress(*this);
        }
        if (!finished()) {
            return false;
        }
        xlog::trace("Background worker finished");
        std::vector<std::string> packfiles = get_pending_packfiles();
        if (packfiles.empty()) {
            if (listener_) {
                listener_->on_finish(*this, false);
            }
        }
        else {
            xlog::trace("Loading packfiles");
            load_packfiles(packfiles);
            if (listener_) {
                listener_->on_finish(*this, true);
            }
        }
        return true;
    }
};

class LevelDownloadManager
{
    std::optional<LevelDownloadOperation> operation_;

public:
    void abort()
    {
        operation_.reset();
    }

    LevelDownloadOperation& start(const char* level_filename, std::unique_ptr<LevelDownloadOperation::Listener>&& listener)
    {
        return operation_.emplace(level_filename, std::move(listener));
    }

    const std::optional<LevelDownloadOperation>& get_operation() const
    {
        return operation_;
    }

    void process()
    {
        if (operation_ && operation_.value().process()) {
            operation_.reset();
        }
    }

    static LevelDownloadManager& instance()
    {
        static LevelDownloadManager inst;
        return inst;
    }
};

class ConsoleReportingDownloadListener : public LevelDownloadOperation::Listener
{
    std::chrono::system_clock::time_point last_progress_print_ = std::chrono::system_clock::now();

public:
    void on_progress(LevelDownloadOperation& operation) override
    {
        if (operation.get_state() == LevelDownloadState::fetching_data) {
            auto now = std::chrono::system_clock::now();
            if (now - last_progress_print_ >= std::chrono::seconds{2}) {
                rf::console::printf("Download progress: %.2f MB / %.2f MB",
                    operation.get_bytes_received() / 1000000.0f,
                    operation.get_level_info().size_in_bytes / 1000000.0f);
                last_progress_print_ = now;
            }
        }
    }

    void on_finish(LevelDownloadOperation& operation, bool success) override
    {
        if (operation.get_state() == LevelDownloadState::not_found) {
            rf::console::printf("Level has not been found in FactionFiles.com database!");
        }
        else {
            rf::console::printf("Level download %s", success ? "succeeded" : "failed");
        }
    }
};

class SetNewLevelStateDownloadListener : public LevelDownloadOperation::Listener
{
public:
    void on_finish(LevelDownloadOperation&, bool) override
    {
        xlog::trace("Changing game state to GS_NEW_LEVEL");
        rf::gameseq_set_state(rf::GS_NEW_LEVEL, false);
    }
};

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

    rf::gr::set_color(0x40, 0x40, 0x40, 0xFF);
    rf::gr::rect(x, y, w, h);

    if (progress_w > 0) {
        rf::gr::set_color(0, 0x80, 0, 0xFF);
        rf::gr::rect(inner_x, inner_y, progress_w, inner_h);
    }

    if (w > progress_w) {
        rf::gr::set_color(0, 0, 0, 0xFF);
        rf::gr::rect(inner_x + progress_w, inner_y, inner_w - progress_w, inner_h);
    }
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

    int scr_w = rf::gr::screen_width();
    int scr_h = rf::gr::screen_height();

    static int bg_bm = rf::bm::load("demo-gameover.tga", -1, false);
    int bg_bm_w, bg_bm_h;
    rf::bm::get_dimensions(bg_bm, &bg_bm_w, &bg_bm_h);
    rf::gr::set_color(255, 255, 255, 255);
    rf::gr::bitmap_scaled(bg_bm, 0, 0, scr_w, scr_h, 0, 0, bg_bm_w, bg_bm_h);

    rf::multi_hud_render_chat();

    auto ccp = &rf::local_player->settings.controls;
    bool just_pressed;
    if (rf::control_config_check_pressed(ccp, rf::CC_ACTION_CHAT, &just_pressed)) {
        rf::multi_chat_say_show(rf::CHAT_SAY_GLOBAL);
    }
    if (rf::multi_chat_is_say_visible()) {
        rf::multi_chat_say_render();
    }

    int large_font = hud_get_large_font();
    int large_font_h = rf::gr::get_font_height(large_font);
    int medium_font = hud_get_default_font();
    int medium_font_h = rf::gr::get_font_height(medium_font);

    int center_x = scr_w / 2;
    int y = scr_h / 3;

    rf::gr::set_color(255, 255, 255, 255);
    rf::gr::string_aligned(rf::gr::ALIGN_CENTER, center_x, y, "DOWNLOADING LEVEL", large_font);
    y += large_font_h * 3;

    const std::optional<LevelDownloadOperation>& operation_opt = LevelDownloadManager::instance().get_operation();
    if (!operation_opt) {
        return;
    }

    auto& operation = operation_opt.value();
    auto state = operation.get_state();
    if (state == LevelDownloadState::fetching_info) {
        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, center_x, y, "Getting level info...", medium_font);
    }
    else if (state == LevelDownloadState::extracting) {
        rf::gr::string_aligned(rf::gr::ALIGN_CENTER, center_x, y, "Extracting packfiles...", medium_font);
    }
    else if (state == LevelDownloadState::fetching_data) {
        const FactionFilesClient::LevelInfo& info = operation.get_level_info();
        unsigned bytes_received = operation.get_bytes_received();
        float bytes_per_sec = operation.get_bytes_per_sec();

        auto level_name_str = std::string{"Level name: "} + info.name;
        int str_w, str_h;
        rf::gr::get_string_size(&str_w, &str_h, level_name_str.c_str(), -1, medium_font);
        int info_x = center_x - str_w / 2;
        int info_spacing = medium_font_h * 3 / 2;
        rf::gr::string(info_x, y, level_name_str.c_str(), medium_font);
        y += info_spacing;

        auto created_by_str = std::string{"Created by: "} + info.author;
        rf::gr::string(info_x, y, created_by_str.c_str(), medium_font);
        y += info_spacing;

        rf::gr::string(info_x, y, string_format("%.2f MB / %.2f MB (%.2f MB/s)",
            bytes_received / 1000.0f / 1000.0f,
            info.size_in_bytes / 1000.0f / 1000.0f,
            bytes_per_sec / 1000.0f / 1000.0f).c_str(), medium_font);
        y += info_spacing;

        if (bytes_per_sec > 0) {
            int remaining_size = info.size_in_bytes - bytes_received;
            int secs_left = remaining_size / bytes_per_sec;
            auto time_left_str = std::to_string(secs_left) + " seconds left";
            rf::gr::string(info_x, y, time_left_str.c_str(), medium_font);
        }

        int w = 360 * rf::ui::scale_x;
        int h = 12 * rf::ui::scale_x;
        int x = (rf::gr::screen_width() - w) / 2;
        int y = rf::gr::screen_height() * 4 / 5;
        float progress = static_cast<float>(bytes_received) / static_cast<float>(info.size_in_bytes);
        render_progress_bar(x, y, w, h, progress);
    }

    // Scoreboard
    if (rf::control_config_check_pressed(ccp, rf::CC_ACTION_MP_STATS, nullptr)) {
        rf::scoreboard_render_internal(true);
    }
}

static bool next_level_exists()
{
    rf::File file;
    if (file.open(rf::level.next_level_filename) == 0) {
        return true;
    }
    return false;
}

CallHook<void(rf::GameState, bool)> process_leave_limbo_packet_gameseq_set_next_state_hook{
    0x0047C24F,
    [](rf::GameState state, bool force) {
        if (!next_level_exists()) {
            rf::gameseq_set_state(rf::GS_MULTI_LEVEL_DOWNLOAD, false);
            LevelDownloadManager::instance().start(rf::level.next_level_filename,
                std::make_unique<SetNewLevelStateDownloadListener>());
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
            LevelDownloadManager::instance().start(rf::level.next_level_filename,
                std::make_unique<SetNewLevelStateDownloadListener>());
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
        if (LevelDownloadManager::instance().get_operation()) {
            xlog::error("Level download is already in progress!");
        }
        else {
            LevelDownloadManager::instance().start(filename.c_str(),
                std::make_unique<ConsoleReportingDownloadListener>());
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
    LevelDownloadManager::instance().process();
}

void multi_level_download_abort()
{
    LevelDownloadManager::instance().abort();
}
