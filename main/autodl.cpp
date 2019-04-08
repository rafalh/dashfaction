#include "autodl.h"
#include "http.h"
#include "misc.h"
#include "rf.h"
#include "rfproto.h"
#include "utils.h"
#include <BuildConfig.h>
#include <RegsPatch.h>
#include <stdexcept>
#include <thread>
#include <unrar/dll.hpp>
#include <unzip.h>
#include <windef.h>

#ifdef LEVELS_AUTODOWNLOADER

#define AUTODL_AGENT_NAME "hoverlees"
#define AUTODL_HOST "pfapi.factionfiles.com"

struct LevelInfo
{
    std::string name;
    std::string author;
    std::string description;
    unsigned size_in_bytes;
    int ticket_id;
};

static LevelInfo g_level_info;
static volatile unsigned g_level_bytes_downloaded;
static volatile bool g_download_in_progress = false;
static std::vector<std::string> g_packfiles_to_load;

static bool IsVppFilename(const char* filename)
{
    const char* ext = strrchr(filename, '.');
    return ext && !stricmp(ext, ".vpp");
}

static bool UnzipVpp(const char* path)
{
    unzFile archive = unzOpen(path);
    if (!archive) {
#ifdef DEBUG
        ERR("unzOpen failed: %s", path);
#endif
        return false;
    }

    bool ret = true;
    unz_global_info global_info;
    int code = unzGetGlobalInfo(archive, &global_info);
    if (code != UNZ_OK) {
        ERR("unzGetGlobalInfo failed - error %d, path %s", code, path);
        std::memset(&global_info, 0, sizeof(global_info));
        ret = false;
    }

    char buf[4096], file_name[MAX_PATH];
    unz_file_info file_info;
    for (unsigned long i = 0; i < global_info.number_entry; i++) {
        code = unzGetCurrentFileInfo(archive, &file_info, file_name, sizeof(file_name), nullptr, 0, nullptr, 0);
        if (code != UNZ_OK) {
            ERR("unzGetCurrentFileInfo failed - error %d, path %s", code, path);
            break;
        }

        if (IsVppFilename(file_name)) {
#ifdef DEBUG
            TRACE("Unpacking %s", file_name);
#endif
            auto output_path = StringFormat("%suser_maps\\multi\\%s", rf::g_RootPath, file_name);
            std::ofstream file(output_path, std::ios_base::out | std::ios_base::binary);
            if (!file) {
                ERR("Cannot open file: %s", output_path.c_str());
                break;
            }

            code = unzOpenCurrentFile(archive);
            if (code != UNZ_OK) {
                ERR("unzOpenCurrentFile failed - error %d, path %s", code, path);
                break;
            }

            while ((code = unzReadCurrentFile(archive, buf, sizeof(buf))) > 0) file.write(buf, code);

            if (code < 0) {
                ERR("unzReadCurrentFile failed - error %d, path %s", code, path);
                break;
            }

            file.close();
            unzCloseCurrentFile(archive);

            g_packfiles_to_load.push_back(file_name);
        }

        if (i + 1 < global_info.number_entry) {
            code = unzGoToNextFile(archive);
            if (code != UNZ_OK) {
                ERR("unzGoToNextFile failed - error %d, path %s", code, path);
                break;
            }
        }
    }

    unzClose(archive);
    return ret;
}

static bool UnrarVpp(const char* path)
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
        ERR("RAROpenArchiveEx failed - result %d, path %s", open_archive_data.OpenResult, path);
        return false;
    }

    bool ret = true;
    struct RARHeaderData header_data;
    header_data.CmtBuf = nullptr;
    memset(&open_archive_data.Reserved, 0, sizeof(open_archive_data.Reserved));

    while (true) {
        int code = RARReadHeader(archive_handle, &header_data);
        if (code == ERAR_END_ARCHIVE)
            break;

        if (code != 0) {
            ERR("RARReadHeader failed - result %d, path %s", code, path);
            break;
        }

        if (IsVppFilename(header_data.FileName)) {
            TRACE("Unpacking %s", header_data.FileName);
            auto output_dir = StringFormat("%suser_maps\\multi", rf::g_RootPath);
            code = RARProcessFile(archive_handle, RAR_EXTRACT, const_cast<char*>(output_dir.c_str()), nullptr);
            if (code == 0) {
                g_packfiles_to_load.push_back(header_data.FileName);
            }
        }
        else {
            TRACE("Skipping %s", header_data.FileName);
            code = RARProcessFile(archive_handle, RAR_SKIP, nullptr, nullptr);
        }

        if (code != 0) {
            ERR("RARProcessFile failed - result %d, path %s", code, path);
            break;
        }
    }

    RARCloseArchive(archive_handle);
    return ret;
}

static bool FetchLevelFile(const char* tmp_filename, int ticket_id) try {
    HttpConnection conn{AUTODL_HOST, 80, AUTODL_AGENT_NAME};

    auto path = StringFormat("downloadmap.php?ticketid=%u", ticket_id);
    HttpRequest req = conn.get(path.c_str());

    std::ofstream tmp_file(tmp_filename, std::ios_base::out | std::ios_base::binary);
    if (!tmp_file) {
        ERR("Cannot open file: %s", tmp_filename);
        return false;
    }

    char buf[4096];
    while (true) {
        auto num_bytes_read = req.read(buf, sizeof(buf));
        if (num_bytes_read <= 0)
            break;
        g_level_bytes_downloaded += num_bytes_read;
        tmp_file.write(buf, num_bytes_read);
    }

    return true;
}
catch (std::exception& e) {
    ERR("%s", e.what());
    return false;
}

static std::optional<std::string> GetTempFileNameInTempDir(const char* prefix)
{
    char temp_dir[MAX_PATH];
    DWORD ret_val = GetTempPathA(std::size(temp_dir), temp_dir);
    if (ret_val == 0 || ret_val > std::size(temp_dir))
        return {};

    char result[MAX_PATH];
    ret_val = GetTempFileNameA(temp_dir, prefix, 0, result);
    if (ret_val == 0)
        return {};

    return {result};
}

static void DownloadLevelThread()
{
    auto temp_filename_opt = GetTempFileNameInTempDir("DF_Level_");
    if (!temp_filename_opt) {
        ERR("Failed to generate a temp filename");
        return;
    }

    auto temp_filename = temp_filename_opt.value().c_str();

    bool success = false;
    if (FetchLevelFile(temp_filename, g_level_info.ticket_id)) {
        if (!UnzipVpp(temp_filename) && !UnrarVpp(temp_filename))
            ERR("UnzipVpp and UnrarVpp failed");
        else
            success = true;
    }

    remove(temp_filename);

    if (!success)
        rf::UiMsgBox("Error!", "Failed to download level file! More information can be found in console.", nullptr,
                     false);

    g_download_in_progress = false;
}

static void StartLevelDownload()
{
    if (g_download_in_progress) {
        ERR("Level download already in progress!");
        return;
    }

    g_level_bytes_downloaded = 0;
    g_download_in_progress = true;

    std::thread download_thread(DownloadLevelThread);
    download_thread.detach();
}

static std::optional<LevelInfo> ParseLevelInfo(char* buf)
{
    std::stringstream ss(buf);
    ss.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string temp;
    std::getline(ss, temp);
    if (temp != "found")
        return {};

    LevelInfo info;
    std::getline(ss, info.name);
    std::getline(ss, info.author);
    std::getline(ss, info.description);

    std::getline(ss, temp);
    info.size_in_bytes = std::stof(temp) * 1024u * 1024u;
    if (!info.size_in_bytes)
        return {};

    std::getline(ss, temp);
    info.ticket_id = std::stoi(temp);
    if (info.ticket_id <= 0)
        return {};

    return {info};
}

static std::optional<LevelInfo> FetchLevelInfo(const char* file_name) try {
    HttpConnection conn{AUTODL_HOST, 80, AUTODL_AGENT_NAME};

    TRACE("Fetching level info: %s", file_name);
    std::string body = std::string("rflName=") + file_name;
    HttpRequest req = conn.post("findmap.php", body);

    char buf[256];
    size_t num_bytes_read = req.read(buf, sizeof(buf) - 1);
    if (num_bytes_read == 0)
        return {};

    buf[num_bytes_read] = '\0';
    TRACE("FactionFiles response: %s", buf);

    return ParseLevelInfo(buf);
}
catch (std::exception& e) {
    ERR("Failed to fetch level info: %s", e.what());
    return {};
}

static void DisplayDownloadDialog(LevelInfo& level_info)
{
    TRACE("Download ticket id: %u", level_info.ticket_id);
    g_level_info = level_info;

    auto msg =
        StringFormat("You don't have needed level: %s (Author: %s, Size: %.2f MB)\nDo you want to download it now?",
                     level_info.name.c_str(), level_info.author.c_str(), level_info.size_in_bytes / 1024.f / 1024.f);
    const char* btn_titles[] = {"Cancel", "Download"};
    rf::UiDialogCallbackPtr callbacks[] = {nullptr, StartLevelDownload};
    rf::UiCreateDialog("Download level", msg.c_str(), 2, btn_titles, callbacks, 0, 0);
}

bool TryToDownloadLevel(const char* filename)
{
    if (g_download_in_progress) {
        TRACE("Level download already in progress!");
        rf::UiMsgBox("Error!", "You can download only one level at once!", nullptr, false);
        return false;
    }

    TRACE("Fetching level info");
    auto level_info_opt = FetchLevelInfo(filename);
    if (!level_info_opt) {
        ERR("Level has not found in FactionFiles database!");
        return false;
    }

    TRACE("Displaying download dialog");
    DisplayDownloadDialog(level_info_opt.value());
    return true;
}

RegsPatch g_join_failed_injection{
    0x0047C4EC,
    [](auto& regs) {
        int leave_reason = regs.esi;
        if (leave_reason != RF_LR_NO_LEVEL_FILE) {
            return;
        }

        auto& level_filename = AddrAsRef<rf::String>(0x00646074);
        TRACE("Preparing level download %s", level_filename.CStr());
        if (!TryToDownloadLevel(level_filename)) {
            return;
        }

        SetJumpToMultiServerList(true);

        regs.eip = 0x0047C502;
        regs.esp -= 0x14;
    },
};

void InitAutodownloader()
{
    g_join_failed_injection.Install();
}

void RenderDownloadProgress()
{
    if (!g_download_in_progress) {
        // Load packages in main thread
        if (!g_packfiles_to_load.empty()) {
            // Load one packfile per frame
            auto& filename = g_packfiles_to_load.front();
            if (!rf::PackfileLoad(filename.c_str(), "user_maps\\multi\\"))
                ERR("PackfileLoad failed - %s", filename.c_str());
            g_packfiles_to_load.erase(g_packfiles_to_load.begin());
        }
        return;
    }

    constexpr int cx = 400, cy = 28;
    float progress = static_cast<float>(g_level_bytes_downloaded) / static_cast<float>(g_level_info.size_in_bytes);
    int cx_progress = static_cast<int>(static_cast<float>(cx) * progress);
    if (cx_progress > cx)
        cx_progress = cx;

    int x = (rf::GrGetMaxWidth() - cx) / 2;
    int y = rf::GrGetMaxHeight() - 50;
    int cy_font = rf::GrGetFontHeight(-1);

    if (cx_progress > 0) {
        rf::GrSetColor(0x80, 0x80, 0, 0x80);
        rf::GrDrawRect(x, y, cx_progress, cy, rf::g_GrRectMaterial);
    }

    if (cx > cx_progress) {
        rf::GrSetColor(0, 0, 0x60, 0x80);
        rf::GrDrawRect(x + cx_progress, y, cx - cx_progress, cy, rf::g_GrRectMaterial);
    }

    rf::GrSetColor(0, 0xFF, 0, 0x80);
    auto text = StringFormat("Downloading: %.2f MB / %.2f MB", g_level_bytes_downloaded / 1024.0f / 1024.0f,
                             g_level_info.size_in_bytes / 1024.0f / 1024.0f);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx / 2, y + cy / 2 - cy_font / 2, text.c_str(), -1,
                          rf::g_GrTextMaterial);
}

#endif /* LEVELS_AUTODOWNLOADER */
