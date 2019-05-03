#include <common/HttpRequest.h>
#include "Exception.h"
#include <sstream>
#include <cassert>
#include <cstring>
#include <string_view>

#define THROW_EXCEPTION_WITH_WIN32_ERROR() THROW_EXCEPTION("win32 error %lu", GetLastError())

struct ParsedUrl
{
    bool ssl;
    std::string host;
    std::string resource;
};

static ParsedUrl parseHttpUrl(std::string_view url)
{
    ParsedUrl result;
    std::string_view http = "http://";
    std::string_view https = "https://";
    size_t host_pos;
    if (url.substr(0, https.size()) == https) {
        result.ssl = true;
        host_pos = https.size();
    }
    else if (url.substr(0, http.size()) == http) {
        result.ssl = false;
        host_pos = http.size();
    }
    else {
        assert(false);
    }

    size_t resource_pos = url.find('/', host_pos);

    if (resource_pos != std::string_view::npos) {
        result.host = url.substr(host_pos, resource_pos - host_pos);
        result.resource = url.substr(resource_pos);
    }
    else {
        result.host = url.substr(host_pos);
        result.resource = "";
    }
    return result;
}

HttpSession::HttpSession(const char* user_agent)
{
    m_inet = {InternetOpenA(user_agent, INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0)};
    if (!m_inet)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpSession::set_connect_timeout(unsigned long timeout_ms)
{
    if (!InternetSetOptionA(m_inet, INTERNET_OPTION_CONNECT_TIMEOUT, const_cast<unsigned long*>(&timeout_ms),
                            sizeof(timeout_ms)))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpSession::set_send_timeout(unsigned long timeout_ms)
{
    if (!InternetSetOptionA(m_inet, INTERNET_OPTION_SEND_TIMEOUT, const_cast<unsigned long*>(&timeout_ms),
                       sizeof(timeout_ms)))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpSession::set_receive_timeout(unsigned long timeout_ms)
{
    if (!InternetSetOptionA(m_inet, INTERNET_OPTION_RECEIVE_TIMEOUT, const_cast<unsigned long*>(&timeout_ms),
                       sizeof(timeout_ms)))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

HttpRequest::HttpRequest(std::string_view url, const char* method, HttpSession& session)
{
    ParsedUrl parsed_url = parseHttpUrl(url);

    HINTERNET inet = session.get_internet_handle();
    int port = parsed_url.ssl ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    m_conn = {InternetConnectA(inet, parsed_url.host.c_str(), port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0)};
    if (!m_conn) {
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (parsed_url.ssl)
        flags |= INTERNET_FLAG_SECURE;

    m_req = {HttpOpenRequestA(m_conn, method, parsed_url.resource.c_str(), "HTTP/1.0", nullptr, nullptr,
                              flags, 0)};
    if (!m_req) {
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    }
}

void HttpRequest::add_header(std::string_view name, std::string_view value)
{
    std::stringstream sstream;
    sstream << name << ": " << value << "\r\n";
    auto str = sstream.str();
    add_raw_headers(str);
}

void HttpRequest::add_raw_headers(std::string_view headers)
{
    HttpAddRequestHeadersA(m_req, headers.data(), headers.size(), 0);
}

void HttpRequest::begin_body(size_t total_body_size)
{
    INTERNET_BUFFERS inet_buffers;
    std::memset(&inet_buffers, 0, sizeof(inet_buffers));
    inet_buffers.dwStructSize = sizeof(inet_buffers);
    inet_buffers.dwBufferTotal = total_body_size;
    if (!HttpSendRequestExA(m_req, &inet_buffers, nullptr, 0, 0))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpRequest::write(const void* data, size_t len)
{
    DWORD written;
    if (!InternetWriteFile(m_req, data, len, &written) || written != len)
        THROW_EXCEPTION_WITH_WIN32_ERROR();
}

void HttpRequest::send(std::string_view body)
{
    if (m_in_body) {
        write(body);
        if (!HttpEndRequestA(m_req, nullptr, 0, 0))
            THROW_EXCEPTION_WITH_WIN32_ERROR();
    }
    else {
        if (!HttpSendRequestA(m_req, nullptr, 0, const_cast<char*>(body.data()), body.size()))
            THROW_EXCEPTION_WITH_WIN32_ERROR();
    }

    DWORD dw_size = sizeof(DWORD);
    DWORD status_code;
    if (!HttpQueryInfoA(m_req, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &dw_size,
                        nullptr)) {
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    }

    if (status_code != 200) {
        throw std::runtime_error(std::string("Invalid HTTP status code: ") + std::to_string(status_code));
    }
}

size_t HttpRequest::read(void* buffer, size_t buffer_size)
{
    DWORD read;
    if (!InternetReadFile(m_req, buffer, buffer_size, &read))
        THROW_EXCEPTION_WITH_WIN32_ERROR();
    return read;
}
