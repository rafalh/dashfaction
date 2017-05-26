#include "HttpRequest.h"
#include "Exception.h"

#define THROW_EXCEPTION_WITH_WIN32_ERROR() THROW_EXCEPTION("win32 error %lu", GetLastError())

PCTSTR HttpRequestInfo::defaultAcceptTypes[] = { TEXT("*/*"), NULL };

HttpRequest::HttpRequest(const HttpRequestInfo &info)
{
    m_inet = InternetOpenA(info.agent.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!m_inet)
        THROW_EXCEPTION_WITH_WIN32_ERROR();

    int port = info.https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    m_conn = InternetConnectA(m_inet, info.host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!m_conn)
    {
        close();
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (info.https)
        flags |= INTERNET_FLAG_SECURE;
    m_req = HttpOpenRequestA(m_conn, info.method, info.path.c_str(), "HTTP/1.1", NULL, info.acceptTypes, flags, 0);
    if (!m_req)
    {
        close();
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    }
}

HttpRequest::~HttpRequest()
{
    close();
}

void HttpRequest::addHeaders(const char *headers)
{
    HttpAddRequestHeadersA(m_req, headers, strlen(headers), 0);
}

void HttpRequest::begin(size_t totalBodySize)
{
    INTERNET_BUFFERS inetBuffers;
    memset(&inetBuffers, 0, sizeof(inetBuffers));
    inetBuffers.dwStructSize = sizeof(inetBuffers);
    inetBuffers.dwBufferTotal = totalBodySize;
    if (!HttpSendRequestExA(m_req, &inetBuffers, NULL, 0, NULL))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpRequest::write(const char *data, size_t len)
{
    DWORD written;
    if (!InternetWriteFile(m_req, data, len, &written) || written != len)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpRequest::end()
{
    if (!HttpEndRequestA(m_req, NULL, 0, NULL))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpRequest::close()
{
    if (m_req)
        InternetCloseHandle(m_req);
    if (m_conn)
        InternetCloseHandle(m_conn);
    if (m_inet)
        InternetCloseHandle(m_inet);
}