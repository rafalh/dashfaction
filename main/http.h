#pragma once

#include <windows.h>
#include <wininet.h>
#include <stdexcept>

class HttpRequest
{
private:
    HINTERNET m_request = nullptr;

public:
    HttpRequest(const HttpRequest&) = delete;

    HttpRequest(HINTERNET connection_handle, const char *path, const char *method = "GET", const std::string &body = "")
    {
        static LPCSTR accept_types[] = {"*/*", nullptr};
        m_request = HttpOpenRequest(connection_handle,
            method, path, nullptr, nullptr, accept_types, INTERNET_FLAG_RELOAD, 0);
        if (!m_request) {
            throw std::runtime_error("HttpOpenRequest failed");
        }

        std::string additional_headers;
        if (!body.empty())
            additional_headers += "Content-Type: application/x-www-form-urlencoded";
        // Note: for some reason HttpSendRequest uses LPVOID parameter for request body but documentation does not say
        // anything about possible modification
        if (!HttpSendRequest(m_request, additional_headers.data(), additional_headers.size(),
                             const_cast<char*>(body.data()), body.size())) {
            throw std::runtime_error("HttpSendRequest failed");
        }

        DWORD dw_size = sizeof(DWORD);
        DWORD status_code;
        if (!HttpQueryInfo(m_request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &dw_size, nullptr)) {
            throw std::runtime_error("HttpQueryInfo failed");
        }

        if (status_code != 200) {
            ERR("Got status code %lu", status_code);
            throw std::runtime_error("Invalid HTTP status code");
        }
    }

    ~HttpRequest()
    {
        if (m_request)
            InternetCloseHandle(m_request);
    }

    size_t read(void *buf, size_t buf_size)
    {
        DWORD bytes_read;
        if (!InternetReadFile(m_request, buf, buf_size, &bytes_read)) {
            ERR("InternetReadFile failed");
            return 0;
        }
        return bytes_read;
    }
};

class HttpConnection
{
private:
    HINTERNET m_internet = nullptr;
    HINTERNET m_connection = nullptr;

public:
    HttpConnection(const HttpConnection&) = delete;

    HttpConnection(const char *host, int port = INTERNET_DEFAULT_HTTP_PORT, const char *agent_name = nullptr)
    {
        m_internet = InternetOpen(agent_name, 0, nullptr, nullptr, 0);
        if (!m_internet) {
            throw std::runtime_error("InternetOpen failed");
        }

        m_connection = InternetConnect(m_internet, host, port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
        if (!m_connection) {
            throw std::runtime_error("InternetConnect failed");
        }
    }

    ~HttpConnection()
    {
        if (m_connection)
            InternetCloseHandle(m_connection);
        if (m_internet)
            InternetCloseHandle(m_internet);
    }

    HINTERNET get_connection_handle() const
    {
        return m_connection;
    }

    HttpRequest request(const char *path, const char *method = "GET", const std::string &body = "")
    {
        return HttpRequest{m_connection, path, method, body};
    }

    HttpRequest get(const char *path)
    {
        return HttpRequest{m_connection, path};
    }

    HttpRequest post(const char *path, const std::string &body)
    {
        return HttpRequest{m_connection, path, "POST", body};
    }
};
