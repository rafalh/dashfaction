#include "UpdateChecker.h"
#include "Exception.h"
#include "version.h"
#include <windows.h>
#include <wininet.h>

#define THROW_EXCEPTION_WITH_WIN32_ERROR() THROW_EXCEPTION("win32 error %lu", GetLastError())

#define UPDATE_URL_DOMAIN "ravin.tk"
#define UPDATE_URL_PATH "api/rf/dashfaction/checkupdate.php"

void UpdateChecker::checkAsync(std::function<void()> callback)
{
    m_abort = false;
    m_callback = callback;
    std::thread th(&UpdateChecker::threadProc, this);
    m_thread = std::move(th);
}

void UpdateChecker::abort()
{
    m_abort = true;
}

bool UpdateChecker::check()
{
    HINTERNET hInternet = NULL, hConnect = NULL, hRequest = NULL;
    char buf[4096];
    LPCTSTR AcceptTypes[] = {TEXT("*/*"), NULL};
    DWORD dwStatus = 0, dwSize = sizeof(DWORD), dwBytesRead;

    try {
        hInternet = InternetOpen("DashFaction", 0, NULL, NULL, 0);
        if (!hInternet)
            THROW_EXCEPTION_WITH_WIN32_ERROR();

        hConnect = InternetConnect(hInternet, UPDATE_URL_DOMAIN, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL,
                                   INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect)
            THROW_EXCEPTION_WITH_WIN32_ERROR();

        sprintf(buf, UPDATE_URL_PATH "?version=%s", VERSION_STR);
        hRequest = HttpOpenRequest(hConnect, NULL, buf, NULL, NULL, AcceptTypes,
                                   INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
        if (!hRequest)
            THROW_EXCEPTION_WITH_WIN32_ERROR();

        if (m_abort)
            return false;
        if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
            THROW_EXCEPTION_WITH_WIN32_ERROR();

        if (m_abort)
            return false;
        if (HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwSize, NULL) &&
            (dwStatus / 100) != 2)
            THROW_EXCEPTION("Invalid status: %lu", dwStatus);

        if (m_abort)
            return false;
        InternetReadFile(hRequest, buf, sizeof(buf), &dwBytesRead);
        buf[dwBytesRead] = 0;

        if (m_abort)
            return false;
        if (!buf[0])
            m_newVersion = false;
        else {
            // printf("%s\n", buf);x
            char* pUrl = buf;
            char* pMsgText = strchr(buf, '\n');
            if (pMsgText) {
                *pMsgText = 0;
                ++pMsgText;
            }
            m_newVersion = true;
            m_url = pUrl;
            m_message = pMsgText;
        }
    }
    catch (const std::exception&) {
        if (hRequest)
            InternetCloseHandle(hRequest);
        if (hConnect)
            InternetCloseHandle(hConnect);
        if (hInternet)
            InternetCloseHandle(hInternet);
        throw;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return m_newVersion;
}

void UpdateChecker::threadProc()
{
    try {
        check();
    }
    catch (const std::exception& e) {
        m_error = e.what();
    }

    if (!m_abort)
        m_callback();
}
