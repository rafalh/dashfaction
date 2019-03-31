#include <BuildConfig.h>
#include <ShortTypes.h>
#include <AsmWritter.h>
#include <windef.h>
#include <unrar/dll.hpp>
#include <unzip.h>
#include <stdexcept>
#include "autodl.h"
#include "rf.h"
#include "rfproto.h"
#include "utils.h"
#include "http.h"

#ifdef LEVELS_AUTODOWNLOADER

#define AUTODL_AGENT_NAME "hoverlees"
#define AUTODL_HOST "pfapi.factionfiles.com"

namespace rf {
static const auto GetJoinFailedReasonStr = AddrAsRef<const char *(unsigned reason)>(0x0047BE60);
}

static unsigned g_LevelTicketId;
static unsigned g_cbLevelSize, g_cbDownloadProgress;
static bool g_DownloadActive = false;

bool UnzipVpp(const char *path)
{
    bool ret = false;
    int code;

    unzFile archive = unzOpen(path);
    if (!archive) {
#ifdef DEBUG
        ERR("unzOpen failed: %s", path);
#endif
        // maybe RAR file
        goto cleanup;
    }

    unz_global_info global_info;
    code = unzGetGlobalInfo(archive, &global_info);
    if (code != UNZ_OK) {
        ERR("unzGetGlobalInfo failed - error %d, path %s", code, path);
        goto cleanup;
    }

    char buf[4096], file_name[MAX_PATH];
    unz_file_info file_info;
    for (int i = 0; i < (int)global_info.number_entry; i++) {
        code = unzGetCurrentFileInfo(archive, &file_info, file_name, sizeof(file_name), NULL, 0, NULL, 0);
        if (code != UNZ_OK) {
            ERR("unzGetCurrentFileInfo failed - error %d, path %s", code, path);
            break;
        }

        const char *ext = strrchr(file_name, '.');
        if (ext && !stricmp(ext, ".vpp"))
        {
#ifdef DEBUG
            TRACE("Unpacking %s", file_name);
#endif
            sprintf(buf, "%suser_maps\\multi\\%s", rf::g_RootPath, file_name);
            FILE *file = fopen(buf, "wb"); /* FIXME: overwrite file? */
            if (!file) {
                ERR("fopen failed - %s", buf);
                break;
            }

            code = unzOpenCurrentFile(archive);
            if (code != UNZ_OK) {
                ERR("unzOpenCurrentFile failed - error %d, path %s", code, path);
                break;
            }

            while ((code = unzReadCurrentFile(archive, buf, sizeof(buf))) > 0)
                fwrite(buf, 1, code, file);

            if (code < 0) {
                ERR("unzReadCurrentFile failed - error %d, path %s", code, path);
                break;
            }

            fclose(file);
            unzCloseCurrentFile(archive);

            if (!rf::PackfileLoad(file_name, "user_maps\\multi\\"))
                ERR("RfLoadVpp failed - %s", file_name);
        }

        if (i + 1 < (int)global_info.number_entry) {
            code = unzGoToNextFile(archive);
            if (code != UNZ_OK) {
                ERR("unzGoToNextFile failed - error %d, path %s", code, path);
                break;
            }
        }
    }

    ret = true;

cleanup:
    if (archive)
        unzClose(archive);
    return ret;
}

bool UnrarVpp(const char *path)
{
    char cmt_buf[16384], buf[256];

    struct RAROpenArchiveDataEx open_archive_data;
    memset(&open_archive_data, 0, sizeof(open_archive_data));
    open_archive_data.ArcName = (char*)path;
    open_archive_data.CmtBuf = cmt_buf;
    open_archive_data.CmtBufSize = sizeof(cmt_buf);
    open_archive_data.OpenMode = RAR_OM_EXTRACT;
    open_archive_data.Callback = NULL;
    HANDLE archive_handle = RAROpenArchiveEx(&open_archive_data);

    if (!archive_handle || open_archive_data.OpenResult != 0) {
        ERR("RAROpenArchiveEx failed - result %d, path %s", open_archive_data.OpenResult, path);
        return false;
    }

    bool ret = true;
    struct RARHeaderData header_data;
    header_data.CmtBuf = NULL;
    memset(&open_archive_data.Reserved, 0, sizeof(open_archive_data.Reserved));

    while (true) {
        int code = RARReadHeader(archive_handle, &header_data);
        if (code == ERAR_END_ARCHIVE)
            break;

        if (code != 0) {
            ERR("RARReadHeader failed - result %d, path %s", code, path);
            break;
        }

        const char *ext = strrchr(header_data.FileName, '.');
        if (ext && !stricmp(ext, ".vpp")) {
            TRACE("Unpacking %s", header_data.FileName);
            sprintf(buf, "%suser_maps\\multi", rf::g_RootPath);
            code = RARProcessFile(archive_handle, RAR_EXTRACT, buf, NULL);
            if (code == 0) {
                if (!rf::PackfileLoad(header_data.FileName, "user_maps\\multi\\"))
                    ERR("RfLoadVpp failed - %s", header_data.FileName);
            }
        }
        else {
            TRACE("Skipping %s", header_data.FileName);
            code = RARProcessFile(archive_handle, RAR_SKIP, NULL, NULL);
        }

        if (code != 0) {
            ERR("RARProcessFile failed - result %d, path %s", code, path);
            break;
        }
    }

    if (archive_handle)
        RARCloseArchive(archive_handle);
    return ret;
}

static bool FetchLevelFile(const char *tmp_file_name) try
{
    HttpConnection conn{AUTODL_HOST, 80, AUTODL_AGENT_NAME};

    char path[64];
    sprintf(path, "downloadmap.php?ticketid=%u", g_LevelTicketId);
    HttpRequest req = conn.request(path);

    FILE *tmp_file = fopen(tmp_file_name, "wb");
    if (!tmp_file) {
        ERR("fopen failed: %s", tmp_file_name);
        return false;
    }

    char buf[4096];
    while (true) {
        auto num_bytes_read = req.read(buf, sizeof(buf));
        if (num_bytes_read <= 0)
            break;
        g_cbDownloadProgress += num_bytes_read;
        fwrite(buf, 1, num_bytes_read, tmp_file);
    }

    g_cbLevelSize = g_cbDownloadProgress;

    fclose(tmp_file);

    return true;
}
catch(std::exception &e)
{
    ERR("%s", e.what());
    return false;
}

static DWORD WINAPI DownloadLevelThread(PVOID param)
{
    char temp_dir[MAX_PATH], temp_file_name[MAX_PATH] = "";
    bool success = false;

    (void)param; // unused parameter

    DWORD dw_ret_val;
    dw_ret_val = GetTempPathA(_countof(temp_dir), temp_dir);
    if (dw_ret_val == 0 || dw_ret_val > _countof(temp_dir)) {
        ERR("GetTempPath failed");
        goto cleanup;
    }

    UINT ret_val;
    ret_val = GetTempFileNameA(temp_dir, "DF_Level", 0, temp_file_name);
    if (ret_val == 0) {
        ERR("GetTempFileName failed");
        goto cleanup;
    }

    if (!FetchLevelFile(temp_file_name))
        goto cleanup;

    if (!UnzipVpp(temp_file_name) && !UnrarVpp(temp_file_name))
        ERR("UnzipVpp and UnrarVpp failed");
    else
        success = true;

cleanup:
    if (temp_file_name[0])
        remove(temp_file_name);

    g_DownloadActive = false;
    if (!success)
        rf::UiMsgBox("Error!", "Failed to download level file! More information can be found in console.", nullptr, false);

    return 0;
}

static void DownloadLevel()
{
    HANDLE thread_handle;

    g_cbDownloadProgress = 0;
    thread_handle = CreateThread(NULL, 0, DownloadLevelThread, NULL, 0, NULL);
    if (thread_handle) {
        CloseHandle(thread_handle);
        g_DownloadActive = true;
    }
    else
        ERR("CreateThread failed");
}

static bool FetchLevelInfo(const char *file_name, char *out_buf, size_t out_buf_size) try
{
    HttpConnection conn{AUTODL_HOST, 80, AUTODL_AGENT_NAME};

    std::string body = std::string("rflName=") + file_name;
    HttpRequest req = conn.request("findmap.php", "POST", body);

    size_t num_bytes_read = req.read(out_buf, out_buf_size - 1);
    if (num_bytes_read == 0)
        return false;

    out_buf[num_bytes_read] = '\0';
    return true;
}
catch(std::exception &e)
{
    ERR("%s", e.what());
    return false;
}

static bool DisplayDownloadDialog(char *buf)
{
    char msg_buf[256], *name, *author, *descr, *size, *ticket_id, *end;
    const char *ppsz_btn_titles[] = {"Cancel", "Download"};
    void *ppfn_callbacks[] = {NULL, (void*)DownloadLevel};

    name = strchr(buf, '\n');
    if (!name)
        return false;
    *(name++) = 0; // terminate first line with 0
    if (strcmp(buf, "found") != 0)
        return false;

    author = strchr(name, '\n');
    if (!author) // terminate name with 0
        return false;
    *(author++) = 0;

    descr = strchr(author, '\n');
    if (!descr)
        return false;
    *(descr++) = 0; // terminate author with 0

    size = strchr(descr, '\n');
    if (!size)
        return false;
    *(size++) = 0; // terminate description with 0

    ticket_id = strchr(size, '\n');
    if (!ticket_id)
        return false;
    *(ticket_id++) = 0; // terminate size with 0

    end = strchr(ticket_id, '\n');
    if (end)
        *end = 0; // terminate ticket id with 0

    g_LevelTicketId = strtoul(ticket_id, NULL, 0);
    g_cbLevelSize = (unsigned)(atof(size) * 1024 * 1024);
    if (!g_LevelTicketId || !g_cbLevelSize)
        return false;

    TRACE("Download ticket id: %u", g_LevelTicketId);

    sprintf(msg_buf, "You don't have needed level: %s (Author: %s, Size: %s MB)\nDo you want to download it now?", name, author, size);
    rf::UiCreateDialog("Download level", msg_buf, 2, ppsz_btn_titles, ppfn_callbacks, 0, 0);
    return true;
}

bool TryToDownloadLevel(const char *file_name)
{
    char buf[256];

    if (g_DownloadActive) {
        rf::UiMsgBox("Error!", "You can download only one level at once!", NULL, FALSE);
        return false;
    }

    if (!FetchLevelInfo(file_name, buf, sizeof(buf))) {
        ERR("Failed to fetch level information");
        return false;
    }

    TRACE("Levels server response: %s", buf);
    if (!DisplayDownloadDialog(buf)) {
        ERR("Failed to parse level information or level not found");
        return false;
    }

    return true;
}

void OnJoinFailed(unsigned reason_id)
{
    if (reason_id == RF_LR_NO_LEVEL_FILE) {
        char *level_file_name = *((char**)0x646078);

        if(TryToDownloadLevel(level_file_name))
            return;
    }

    const char *reason_str = rf::GetJoinFailedReasonStr(reason_id);
    rf::UiMsgBox(rf::strings::exiting_game, reason_str, NULL, 0);
}

void InitAutodownloader()
{
    AsmWritter(0x0047C4ED).call(OnJoinFailed);
    AsmWritter(0x0047C4FD).nop(5);
}

void RenderDownloadProgress()
{
    char buf[256];
    unsigned x, y, cx_progress, cy_font;
    const unsigned cx = 400, cy = 28;

    if (!g_DownloadActive)
        return;

    cx_progress = (unsigned)((float)cx * (float)g_cbDownloadProgress / (float)g_cbLevelSize);
    if (cx_progress > cx)
        cx_progress = cx;

    x = (rf::GrGetMaxWidth() - cx) / 2;
    y = rf::GrGetMaxHeight() - 50;
    cy_font = rf::GrGetFontHeight(-1);

    if (cx_progress > 0) {
        rf::GrSetColor(0x80, 0x80, 0, 0x80);
        rf::GrDrawRect(x, y, cx_progress, cy, rf::g_GrRectMaterial);
    }

    if (cx > cx_progress) {
        rf::GrSetColor(0, 0, 0x60, 0x80);
        rf::GrDrawRect(x + cx_progress, y, cx - cx_progress, cy, rf::g_GrRectMaterial);
    }

    rf::GrSetColor(0, 0xFF, 0, 0x80);
    std::sprintf(buf, "Downloading: %.2f MB / %.2f MB", g_cbDownloadProgress/1024.0f/1024.0f, g_cbLevelSize/1024.0f/1024.0f);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx/2, y + cy/2 - cy_font/2, buf, -1, rf::g_GrTextMaterial);
}

#endif /* LEVELS_AUTODOWNLOADER */
