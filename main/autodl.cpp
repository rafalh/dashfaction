#include <BuildConfig.h>
#include <ShortTypes.h>
#include <AsmWritter.h>
#include <windows.h>
#include <wininet.h>
#include <unrar/dll.hpp>
#include <unzip.h>
#include "autodl.h"
#include "rf.h"
#include "rfproto.h"
#include "utils.h"

#ifdef LEVELS_AUTODOWNLOADER

#define AUTODL_AGENT_NAME "hoverlees"
#define AUTODL_HOST "pfapi.factionfiles.com"

namespace rf {
static const auto GetJoinFailedReasonStr = AddrAsRef<const char *(unsigned Reason)>(0x0047BE60);
}

static unsigned g_LevelTicketId;
static unsigned g_cbLevelSize, g_cbDownloadProgress;
static bool g_DownloadActive = false;

bool UnzipVpp(const char *Path)
{
    bool Ret = false;
    int Code;

    unzFile Archive = unzOpen(Path);
    if (!Archive) {
#ifdef DEBUG
        ERR("unzOpen failed: %s", Path);
#endif
        // maybe RAR file
        goto cleanup;
    }

    unz_global_info GlobalInfo;
    Code = unzGetGlobalInfo(Archive, &GlobalInfo);
    if (Code != UNZ_OK) {
        ERR("unzGetGlobalInfo failed - error %d, path %s", Code, Path);
        goto cleanup;
    }

    char Buf[4096], FileName[MAX_PATH];
    unz_file_info FileInfo;
    for (int i = 0; i < (int)GlobalInfo.number_entry; i++) {
        Code = unzGetCurrentFileInfo(Archive, &FileInfo, FileName, sizeof(FileName), NULL, 0, NULL, 0);
        if (Code != UNZ_OK) {
            ERR("unzGetCurrentFileInfo failed - error %d, path %s", Code, Path);
            break;
        }

        const char *Ext = strrchr(FileName, '.');
        if (Ext && !stricmp(Ext, ".vpp"))
        {
#ifdef DEBUG
            TRACE("Unpacking %s", FileName);
#endif
            sprintf(Buf, "%suser_maps\\multi\\%s", rf::g_RootPath, FileName);
            FILE *File = fopen(Buf, "wb"); /* FIXME: overwrite file? */
            if (!File) {
                ERR("fopen failed - %s", Buf);
                break;
            }

            Code = unzOpenCurrentFile(Archive);
            if (Code != UNZ_OK) {
                ERR("unzOpenCurrentFile failed - error %d, path %s", Code, Path);
                break;
            }

            while ((Code = unzReadCurrentFile(Archive, Buf, sizeof(Buf))) > 0)
                fwrite(Buf, 1, Code, File);

            if (Code < 0) {
                ERR("unzReadCurrentFile failed - error %d, path %s", Code, Path);
                break;
            }

            fclose(File);
            unzCloseCurrentFile(Archive);

            if (!rf::PackfileLoad(FileName, "user_maps\\multi\\"))
                ERR("RfLoadVpp failed - %s", FileName);
        }

        if (i + 1 < (int)GlobalInfo.number_entry) {
            Code = unzGoToNextFile(Archive);
            if (Code != UNZ_OK) {
                ERR("unzGoToNextFile failed - error %d, path %s", Code, Path);
                break;
            }
        }
    }

    Ret = true;

cleanup:
    if (Archive)
        unzClose(Archive);
    return Ret;
}

bool UnrarVpp(const char *Path)
{
    char CmtBuf[16384], Buf[256];

    struct RAROpenArchiveDataEx OpenArchiveData;
    memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
    OpenArchiveData.ArcName = (char*)Path;
    OpenArchiveData.CmtBuf = CmtBuf;
    OpenArchiveData.CmtBufSize = sizeof(CmtBuf);
    OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
    OpenArchiveData.Callback = NULL;
    HANDLE Archive_handle = RAROpenArchiveEx(&OpenArchiveData);

    if (!Archive_handle || OpenArchiveData.OpenResult != 0) {
        ERR("RAROpenArchiveEx failed - result %d, path %s", OpenArchiveData.OpenResult, Path);
        return false;
    }

    bool Ret = true;
    struct RARHeaderData HeaderData;
    HeaderData.CmtBuf = NULL;
    memset(&OpenArchiveData.Reserved, 0, sizeof(OpenArchiveData.Reserved));

    while (true) {
        int Code = RARReadHeader(Archive_handle, &HeaderData);
        if (Code == ERAR_END_ARCHIVE)
            break;

        if (Code != 0) {
            ERR("RARReadHeader failed - result %d, path %s", Code, Path);
            break;
        }

        const char *Ext = strrchr(HeaderData.FileName, '.');
        if (Ext && !stricmp(Ext, ".vpp")) {
            TRACE("Unpacking %s", HeaderData.FileName);
            sprintf(Buf, "%suser_maps\\multi", rf::g_RootPath);
            Code = RARProcessFile(Archive_handle, RAR_EXTRACT, Buf, NULL);
            if (Code == 0) {
                if (!rf::PackfileLoad(HeaderData.FileName, "user_maps\\multi\\"))
                    ERR("RfLoadVpp failed - %s", HeaderData.FileName);
            }
        }
        else {
            TRACE("Skipping %s", HeaderData.FileName);
            Code = RARProcessFile(Archive_handle, RAR_SKIP, NULL, NULL);
        }

        if (Code != 0) {
            ERR("RARProcessFile failed - result %d, path %s", Code, Path);
            break;
        }
    }

    if (Archive_handle)
        RARCloseArchive(Archive_handle);
    return Ret;
}

static bool FetchLevelFile(const char *TmpFileName)
{
    HINTERNET Internet_handle = NULL, Connect_handle = NULL, Request_handle = NULL;
    char buf[4096];
    LPCTSTR AcceptTypes[] = {TEXT("*/*"), NULL};
    DWORD dwStatus = 0, dwSize = sizeof(DWORD), dwBytesRead;
    FILE *TmpFile;
    bool Success = false;

    Internet_handle = InternetOpen(AUTODL_AGENT_NAME, 0, NULL, NULL, 0);
    if (!Internet_handle)
        goto cleanup;

    Connect_handle = InternetConnect(Internet_handle, AUTODL_HOST, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!Connect_handle) {
        ERR("InternetConnect failed");
        goto cleanup;
    }

    sprintf(buf, "downloadmap.php?ticketid=%u", g_LevelTicketId);
    Request_handle = HttpOpenRequest(Connect_handle, NULL, buf, NULL, NULL, AcceptTypes, INTERNET_FLAG_RELOAD, 0);
    if (!Request_handle) {
        ERR("HttpOpenRequest failed");
        goto cleanup;
    }

    if (!HttpSendRequest(Request_handle, NULL, 0, NULL, 0)) {
        ERR("HttpSendRequest failed");
        goto cleanup;
    }

    if (HttpQueryInfo(Request_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwSize, NULL) && (dwStatus / 100) != 2) {
        ERR("HttpQueryInfo failed or status code (%lu) is wrong", dwStatus);
        goto cleanup;
    }

    TmpFile = fopen(TmpFileName, "wb");
    if (!TmpFile) {
        ERR("fopen failed: %s", TmpFileName);
        goto cleanup;
    }

    while (InternetReadFile(Request_handle, buf, sizeof(buf), &dwBytesRead) && dwBytesRead > 0) {
        g_cbDownloadProgress += dwBytesRead;
        fwrite(buf, 1, dwBytesRead, TmpFile);
    }

    g_cbLevelSize = g_cbDownloadProgress;

    fclose(TmpFile);
    Success = true;

cleanup:
    if (Request_handle)
        InternetCloseHandle(Request_handle);
    if (Connect_handle)
        InternetCloseHandle(Connect_handle);
    if (Internet_handle)
        InternetCloseHandle(Internet_handle);

    return Success;
}

static DWORD WINAPI DownloadLevelThread(PVOID Param)
{
    char TempDir[MAX_PATH], TempFileName[MAX_PATH] = "";
    bool Success = false;

    (void)Param; // unused parameter

    DWORD dwRetVal;
    dwRetVal = GetTempPathA(_countof(TempDir), TempDir);
    if (dwRetVal == 0 || dwRetVal > _countof(TempDir)) {
        ERR("GetTempPath failed");
        goto cleanup;
    }

    UINT RetVal;
    RetVal = GetTempFileNameA(TempDir, "DF_Level", 0, TempFileName);
    if (RetVal == 0) {
        ERR("GetTempFileName failed");
        goto cleanup;
    }

    if (!FetchLevelFile(TempFileName))
        goto cleanup;

    if (!UnzipVpp(TempFileName) && !UnrarVpp(TempFileName))
        ERR("UnzipVpp and UnrarVpp failed");
    else
        Success = true;

cleanup:
    if (TempFileName[0])
        remove(TempFileName);

    g_DownloadActive = false;
    if (!Success)
        rf::UiMsgBox("Error!", "Failed to download level file! More information can be found in console.", nullptr, false);

    return 0;
}

static void DownloadLevel()
{
    HANDLE Thread_handle;

    g_cbDownloadProgress = 0;
    Thread_handle = CreateThread(NULL, 0, DownloadLevelThread, NULL, 0, NULL);
    if (Thread_handle) {
        CloseHandle(Thread_handle);
        g_DownloadActive = true;
    }
    else
        ERR("CreateThread failed");
}

static bool FetchLevelInfo(const char *FileName, char *OutBuf, size_t OutBufSize)
{
    HINTERNET Internet_handle = NULL, Connect_handle = NULL, Request_handle = NULL;
    char Buf[256];
    static LPCSTR AcceptTypes[] = {"*/*", NULL};
    static char Headers[] = "Content-Type: application/x-www-form-urlencoded";
    DWORD dwBytesRead, dwStatus = 0, dwSize = sizeof(DWORD);
    bool Ret = false;

    Internet_handle = InternetOpen(AUTODL_AGENT_NAME, 0, NULL, NULL, 0);
    if (!Internet_handle) {
        ERR("InternetOpen failed");
        goto cleanup;
    }

    Connect_handle = InternetConnect(Internet_handle, AUTODL_HOST, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!Connect_handle) {
        ERR("InternetConnect failed");
        goto cleanup;
    }

    Request_handle = HttpOpenRequest(Connect_handle, "POST", "findmap.php", NULL, NULL, AcceptTypes, INTERNET_FLAG_RELOAD, 0);
    if (!Request_handle) {
        ERR("HttpOpenRequest failed");
        goto cleanup;
    }

    dwSize = sprintf(Buf, "rflName=%s", FileName);
    if (!HttpSendRequest(Request_handle, Headers, sizeof(Headers) - 1, Buf, dwSize)) {
        ERR("HttpSendRequest failed");
        goto cleanup;
    }

    if (HttpQueryInfo(Request_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwSize, NULL) && (dwStatus / 100) != 2) {
        ERR("HttpQueryInfo failed or status code (%lu) is wrong", dwStatus);
        goto cleanup;
    }

    if (!InternetReadFile(Request_handle, OutBuf, OutBufSize - 1, &dwBytesRead)) {
        ERR("InternetReadFile failed", NULL);
        goto cleanup;
    }

    OutBuf[dwBytesRead] = '\0';
    Ret = true;

cleanup:
    if (Request_handle)
        InternetCloseHandle(Request_handle);
    if (Connect_handle)
        InternetCloseHandle(Connect_handle);
    if (Internet_handle)
        InternetCloseHandle(Internet_handle);

    return Ret;
}

static bool DisplayDownloadDialog(char *Buf)
{
    char MsgBuf[256], *Name, *Author, *Descr, *Size, *TicketId, *End;
    const char *ppszBtnTitles[] = {"Cancel", "Download"};
    void *ppfnCallbacks[] = {NULL, (void*)DownloadLevel};

    Name = strchr(Buf, '\n');
    if (!Name)
        return false;
    *(Name++) = 0; // terminate first line with 0
    if (strcmp(Buf, "found") != 0)
        return false;

    Author = strchr(Name, '\n');
    if (!Author) // terminate name with 0
        return false;
    *(Author++) = 0;

    Descr = strchr(Author, '\n');
    if (!Descr)
        return false;
    *(Descr++) = 0; // terminate author with 0

    Size = strchr(Descr, '\n');
    if (!Size)
        return false;
    *(Size++) = 0; // terminate description with 0

    TicketId = strchr(Size, '\n');
    if (!TicketId)
        return false;
    *(TicketId++) = 0; // terminate size with 0

    End = strchr(TicketId, '\n');
    if (End)
        *End = 0; // terminate ticket id with 0

    g_LevelTicketId = strtoul(TicketId, NULL, 0);
    g_cbLevelSize = (unsigned)(atof(Size) * 1024 * 1024);
    if (!g_LevelTicketId || !g_cbLevelSize)
        return false;

    TRACE("Download ticket id: %u", g_LevelTicketId);

    sprintf(MsgBuf, "You don't have needed level: %s (Author: %s, Size: %s MB)\nDo you want to download it now?", Name, Author, Size);
    rf::UiCreateDialog("Download level", MsgBuf, 2, ppszBtnTitles, ppfnCallbacks, 0, 0);
    return true;
}

bool TryToDownloadLevel(const char *FileName)
{
    char Buf[256];

    if (g_DownloadActive) {
        rf::UiMsgBox("Error!", "You can download only one level at once!", NULL, FALSE);
        return false;
    }

    if (!FetchLevelInfo(FileName, Buf, sizeof(Buf))) {
        ERR("Failed to fetch level information");
        return false;
    }

    TRACE("Levels server response: %s", Buf);
    if (!DisplayDownloadDialog(Buf)) {
        ERR("Failed to parse level information or level not found");
        return false;
    }

    return true;
}

void OnJoinFailed(unsigned ReasonId)
{
    if (ReasonId == RF_LR_NO_LEVEL_FILE) {
        char *LevelFileName = *((char**)0x646078);

        if(TryToDownloadLevel(LevelFileName))
            return;
    }

    const char *ReasonStr = rf::GetJoinFailedReasonStr(ReasonId);
    rf::UiMsgBox(rf::strings::exiting_game, ReasonStr, NULL, 0);
}

void InitAutodownloader()
{
    AsmWritter(0x0047C4ED).call(OnJoinFailed);
    AsmWritter(0x0047C4FD).nop(5);
}

void RenderDownloadProgress()
{
    char Buf[256];
    unsigned x, y, cxProgress, cyFont;
    const unsigned cx = 400, cy = 28;

    if (!g_DownloadActive)
        return;

    cxProgress = (unsigned)((float)cx * (float)g_cbDownloadProgress / (float)g_cbLevelSize);
    if (cxProgress > cx)
        cxProgress = cx;

    x = (rf::GrGetMaxWidth() - cx) / 2;
    y = rf::GrGetMaxHeight() - 50;
    cyFont = rf::GrGetFontHeight(-1);

    if (cxProgress > 0) {
        rf::GrSetColor(0x80, 0x80, 0, 0x80);
        rf::GrDrawRect(x, y, cxProgress, cy, rf::g_GrRectMaterial);
    }

    if (cx > cxProgress) {
        rf::GrSetColor(0, 0, 0x60, 0x80);
        rf::GrDrawRect(x + cxProgress, y, cx - cxProgress, cy, rf::g_GrRectMaterial);
    }

    rf::GrSetColor(0, 0xFF, 0, 0x80);
    std::sprintf(Buf, "Downloading: %.2f MB / %.2f MB", g_cbDownloadProgress/1024.0f/1024.0f, g_cbLevelSize/1024.0f/1024.0f);
    rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, x + cx/2, y + cy/2 - cyFont/2, Buf, -1, rf::g_GrTextMaterial);
}

#endif /* LEVELS_AUTODOWNLOADER */
