#include "stdafx.h"
#include "autodl.h"
#include "rf.h"
#include "rfproto.h"
#include "utils.h"

#ifdef LEVELS_AUTODOWNLOADER

#define AUTODL_AGENT_NAME "hoverlees"
#define AUTODL_HOST "pfapi.factionfiles.com"

using namespace rf;

static unsigned g_LevelTicketId;
static unsigned g_cbLevelSize, g_cbDownloadProgress;
static BOOL g_bDownloadActive = FALSE;

bool UnzipVpp(const char *pszPath)
{
    bool bRet = false;

    unzFile Archive = unzOpen(pszPath);
    if (!Archive)
    {
#ifdef DEBUG
        ERR("unzOpen failed: %s", pszPath);
#endif
        // maybe RAR file
        goto cleanup;
    }

    unz_global_info GlobalInfo;
    int iCode = unzGetGlobalInfo(Archive, &GlobalInfo);
    if (iCode != UNZ_OK)
    {
        ERR("unzGetGlobalInfo failed - error %d, path %s", iCode, pszPath);
        goto cleanup;
    }

    char Buf[4096], szFileName[MAX_PATH];
    unz_file_info FileInfo;
    for (int i = 0; i < (int)GlobalInfo.number_entry; i++)
    {
        iCode = unzGetCurrentFileInfo(Archive, &FileInfo, szFileName, sizeof(szFileName), NULL, 0, NULL, 0);
        if (iCode != UNZ_OK)
        {
            ERR("unzGetCurrentFileInfo failed - error %d, path %s", iCode, pszPath);
            break;
        }

        const char *pszExt = strrchr(szFileName, '.');
        if (pszExt && !stricmp(pszExt, ".vpp"))
        {
#ifdef DEBUG
            TRACE("Unpacking %s", szFileName);
#endif
            sprintf(Buf, "%suser_maps\\multi\\%s", g_pszRootPath, szFileName);
            FILE *pFile = fopen(Buf, "wb"); /* FIXME: overwrite file? */
            if (!pFile)
            {
                ERR("fopen failed - %s", Buf);
                break;
            }

            iCode = unzOpenCurrentFile(Archive);
            if (iCode != UNZ_OK)
            {
                ERR("unzOpenCurrentFile failed - error %d, path %s", iCode, pszPath);
                break;
            }

            while ((iCode = unzReadCurrentFile(Archive, Buf, sizeof(Buf))) > 0)
                fwrite(Buf, 1, iCode, pFile);

            if (iCode < 0)
            {
                ERR("unzReadCurrentFile failed - error %d, path %s", iCode, pszPath);
                break;
            }

            fclose(pFile);
            unzCloseCurrentFile(Archive);

            if (!PackfileLoad(szFileName, "user_maps\\multi\\"))
                ERR("RfLoadVpp failed - %s", szFileName);
        }

        if (i + 1 < (int)GlobalInfo.number_entry)
        {
            iCode = unzGoToNextFile(Archive);
            if (iCode != UNZ_OK)
            {
                ERR("unzGoToNextFile failed - error %d, path %s", iCode, pszPath);
                break;
            }
        }
    }

    bRet = true;

cleanup:
    if (Archive)
        unzClose(Archive);
    return bRet;
}

bool UnrarVpp(const char *pszPath)
{
    char CmtBuf[16384], szBuf[256];

    struct RAROpenArchiveDataEx OpenArchiveData;
    memset(&OpenArchiveData, 0, sizeof(OpenArchiveData));
    OpenArchiveData.ArcName = (char*)pszPath;
    OpenArchiveData.CmtBuf = CmtBuf;
    OpenArchiveData.CmtBufSize = sizeof(CmtBuf);
    OpenArchiveData.OpenMode = RAR_OM_EXTRACT;
    OpenArchiveData.Callback = NULL;
    HANDLE hArchive = RAROpenArchiveEx(&OpenArchiveData);

    if (!hArchive || OpenArchiveData.OpenResult != 0)
    {
        ERR("RAROpenArchiveEx failed - result %d, path %s", OpenArchiveData.OpenResult, pszPath);
        return false;
    }

    bool bRet = true;
    struct RARHeaderData HeaderData;
    HeaderData.CmtBuf = NULL;
    memset(&OpenArchiveData.Reserved, 0, sizeof(OpenArchiveData.Reserved));

    while (true)
    {
        int Code = RARReadHeader(hArchive, &HeaderData);
        if (Code == ERAR_END_ARCHIVE)
            break;

        if (Code != 0)
        {
            ERR("RARReadHeader failed - result %d, path %s", Code, pszPath);
            break;
        }

        const char *pszExt = strrchr(HeaderData.FileName, '.');
        if (pszExt && !stricmp(pszExt, ".vpp"))
        {
            TRACE("Unpacking %s", HeaderData.FileName);
            sprintf(szBuf, "%suser_maps\\multi", g_pszRootPath);
            Code = RARProcessFile(hArchive, RAR_EXTRACT, szBuf, NULL);
            if (Code == 0)
            {
                if (!PackfileLoad(HeaderData.FileName, "user_maps\\multi\\"))
                    ERR("RfLoadVpp failed - %s", HeaderData.FileName);
            }
        }
        else
        {
            TRACE("Skipping %s", HeaderData.FileName);
            Code = RARProcessFile(hArchive, RAR_SKIP, NULL, NULL);
        }
        
        if (Code != 0)
        {
            ERR("RARProcessFile failed - result %d, path %s", Code, pszPath);
            break;
        }
    }

    if (hArchive)
        RARCloseArchive(hArchive);
    return bRet;
}

static DWORD WINAPI DownloadLevelThread(PVOID pParam)
{
    HINTERNET hInternet = NULL, hConnect = NULL, hRequest = NULL;
    char buf[4096], szTmpName[L_tmpnam];
    LPCTSTR AcceptTypes[] = {TEXT("*/*"), NULL};
    DWORD dwStatus = 0, dwSize = sizeof(DWORD), dwBytesRead;
    FILE *pTmpFile;

    hInternet = InternetOpen(AUTODL_AGENT_NAME, 0, NULL, NULL, 0);
    if (!hInternet)
        goto cleanup;

    hConnect = InternetConnect(hInternet, AUTODL_HOST, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect)
    {
        ERR("InternetConnect failed");
        goto cleanup;
    }

    sprintf(buf, "downloadmap.php?ticketid=%u", g_LevelTicketId);
    hRequest = HttpOpenRequest(hConnect, NULL, buf, NULL, NULL, AcceptTypes, INTERNET_FLAG_RELOAD, 0);
    if (!hRequest)
    {
        ERR("HttpOpenRequest failed");
        goto cleanup;
    }

    if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
    {
        ERR("HttpSendRequest failed");
        goto cleanup;
    }

    if (HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwSize, NULL) && (dwStatus / 100) != 2)
    {
        ERR("HttpQueryInfo failed or status code (%lu) is wrong", dwStatus);
        goto cleanup;
    }

    tmpnam(szTmpName);
    pTmpFile = fopen(szTmpName, "wb");
    if (!pTmpFile)
    {
        ERR("fopen failed");
        goto cleanup;
    }

    while (InternetReadFile(hRequest, buf, sizeof(buf), &dwBytesRead) && dwBytesRead > 0)
    {
        g_cbDownloadProgress += dwBytesRead;
        fwrite(buf, 1, dwBytesRead, pTmpFile);
    }

    g_cbLevelSize = g_cbDownloadProgress;

    fclose(pTmpFile);

    if (!UnzipVpp(szTmpName) && !UnrarVpp(szTmpName))
        ERR("UnzipVpp and UnrarVpp failed");

    remove(szTmpName);

    g_bDownloadActive = FALSE;

cleanup:
    if (hRequest)
        InternetCloseHandle(hRequest);
    if (hConnect)
        InternetCloseHandle(hConnect);
    if (hInternet)
        InternetCloseHandle(hInternet);
    return 0;
}

static void DownloadLevel(void)
{
    HANDLE hThread;

    hThread = CreateThread(NULL, 0, DownloadLevelThread, NULL, 0, NULL);
    if (hThread)
    {
        CloseHandle(hThread);
        g_cbDownloadProgress = 0;
        g_bDownloadActive = TRUE;
    }
    else
        ERR("CreateThread failed");
}

BOOL TryToDownloadLevel(const char *pszFileName)
{
    HINTERNET hInternet = NULL, hConnect = NULL, hRequest = NULL;
    char szBuf[256], szMsgBuf[256], *pszName, *pszAuthor, *pszDescr, *pszSize, *pszTicketId, *pszEnd;
    static LPCSTR AcceptTypes[] = {"*/*", NULL};
    static char szHeaders[] = "Content-Type: application/x-www-form-urlencoded";
    DWORD dwBytesRead, dwStatus = 0, dwSize = sizeof(DWORD);
    BOOL bRet = FALSE;
    const char *ppszBtnTitles[] = {"Cancel", "Download"};
    void *ppfnCallbacks[] = {NULL, (void*)DownloadLevel};

    if (g_bDownloadActive)
    {
        rf::UiMsgBox("Error!", "You can download only one level at once!", NULL, FALSE);
        return FALSE;
    }

    hInternet = InternetOpen(AUTODL_AGENT_NAME, 0, NULL, NULL, 0);
    if (!hInternet)
    {
        ERR("InternetOpen failed");
        goto cleanup;
    }

    hConnect = InternetConnect(hInternet, AUTODL_HOST, INTERNET_DEFAULT_HTTP_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect)
    {
        ERR("InternetConnect failed");
        goto cleanup;
    }

    hRequest = HttpOpenRequest(hConnect, "POST", "findmap.php", NULL, NULL, AcceptTypes, INTERNET_FLAG_RELOAD, 0);
    if (!hRequest)
    {
        ERR("HttpOpenRequest failed");
        goto cleanup;
    }

    dwSize = sprintf(szBuf, "rflName=%s", pszFileName);
    if (!HttpSendRequest(hRequest, szHeaders, sizeof(szHeaders) - 1, szBuf, dwSize))
    {
        ERR("HttpSendRequest failed");
        goto cleanup;
    }

    if (HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwSize, NULL) && (dwStatus / 100) != 2)
    {
        ERR("HttpQueryInfo failed or status code (%lu) is wrong", dwStatus);
        goto cleanup;
    }

    if (!InternetReadFile(hRequest, szBuf, sizeof(szBuf) - 1, &dwBytesRead))
    {
        ERR("InternetReadFile failed", NULL);
        goto cleanup;
    }

    szBuf[dwBytesRead] = '\0';

    TRACE("Maps server response: %s", szBuf);

    pszName = strchr(szBuf, '\n');
    if (!pszName)
        goto cleanup;
    *(pszName++) = 0; // terminate first line with 0
    if (strcmp(szBuf, "found") != 0)
        goto cleanup;

    pszAuthor = strchr(pszName, '\n');
    if (!pszAuthor) // terminate name with 0
        goto cleanup;
    *(pszAuthor++) = 0;

    pszDescr = strchr(pszAuthor, '\n');
    if (!pszDescr)
        goto cleanup;
    *(pszDescr++) = 0; // terminate author with 0

    pszSize = strchr(pszDescr, '\n');
    if (!pszSize)
        goto cleanup;
    *(pszSize++) = 0; // terminate description with 0

    pszTicketId = strchr(pszSize, '\n');
    if (!pszTicketId)
        goto cleanup;
    *(pszTicketId++) = 0; // terminate size with 0

    pszEnd = strchr(pszTicketId, '\n');
    if (pszEnd)
        *pszEnd = 0; // terminate ticket id with 0

    g_LevelTicketId = strtoul(pszTicketId, NULL, 0);
    g_cbLevelSize = (unsigned)(atof(pszSize) * 1024 * 1024);
    if (!g_LevelTicketId || !g_cbLevelSize)
        goto cleanup;

    TRACE("Download ticket id: %u", g_LevelTicketId);

    sprintf(szMsgBuf, "You don't have needed level: %s (Author: %s, Size: %s MB)\nDo you want to download it now?", pszName, pszAuthor, pszSize);
    UiCreateDialog("Download level", szMsgBuf, 2, ppszBtnTitles, ppfnCallbacks, 0, 0);
    bRet = TRUE;

cleanup:
    if (hRequest)
        InternetCloseHandle(hRequest);
    if (hConnect)
        InternetCloseHandle(hConnect);
    if (hInternet)
        InternetCloseHandle(hInternet);
    return bRet;
}

void OnJoinFailed(unsigned Reason)
{
    const char *pszReason;

    if (Reason == RF_LR_NO_LEVEL_FILE)
    {
        char *pszLevelFileName = *((char**)0x646078);

        if(TryToDownloadLevel(pszLevelFileName))
            return;
    }

    pszReason = rf::GetJoinFailedStr(Reason);
    rf::UiMsgBox(g_ppszStringsTable[STR_EXITING_GAME], pszReason, NULL, 0);
}

void InitAutodownloader(void)
{
    WriteMemInt32(0x0047C4EE, (uintptr_t)OnJoinFailed - (0x0047C4ED + 5));
    WriteMemUInt8(0x0047C4FD, ASM_NOP, 5);
}

void RenderDownloadProgress(void)
{
    char szBuf[256];
    unsigned x, y, cxProgress, cyFont;
    const unsigned cx = 400, cy = 28;

    if (!g_bDownloadActive)
        return;

    cxProgress = (unsigned)((float)cx * (float)g_cbDownloadProgress / (float)g_cbLevelSize);
    if (cxProgress > cx)
        cxProgress = cx;

    x = (GrGetMaxWidth() - cx) / 2;
    y = GrGetMaxHeight() - 50;
    cyFont = GrGetFontHeight(-1);

    if (cxProgress > 0)
    {
        GrSetColor(0x80, 0x80, 0, 0x80);
        GrDrawRect(x, y, cxProgress, cy, *g_pGrRectMaterial);
    }

    if (cx > cxProgress)
    {
        GrSetColor(0, 0, 0x60, 0x80);
        GrDrawRect(x + cxProgress, y, cx - cxProgress, cy, *g_pGrRectMaterial);
    }

    GrSetColor(0, 0xFF, 0, 0x80);
    sprintf(szBuf, "Downloading: %.2f MB / %.2f MB", g_cbDownloadProgress/1024.0f/1024.0f, g_cbLevelSize/1024.0f/1024.0f);
    GrDrawAlignedText(GR_ALIGN_CENTER, x + cx/2, y + cy/2 - cyFont/2, szBuf, -1, *g_pGrTextMaterial);
}

#endif /* LEVELS_AUTODOWNLOADER */
