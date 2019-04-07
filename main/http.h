#pragma once

#include <stdexcept>
#include <windows.h>
#include <wininet.h>
#include <log/Logger.h>

class InternetHandle
{
private:
    HINTERNET m_handle;

public:
    InternetHandle() :
        m_handle(nullptr)
    {}

    ~InternetHandle()
    {
        if (m_handle)
            InternetCloseHandle(m_handle);
    }

    InternetHandle(const InternetHandle&) = delete; // copy constructor
    InternetHandle& operator=(const InternetHandle&) = delete;

    InternetHandle(InternetHandle&& other) noexcept : // move constructor
        m_handle(std::exchange(other.m_handle, nullptr))
    {}

    InternetHandle& operator=(InternetHandle&& other) noexcept // move assignment
    {
        std::swap(m_handle, other.m_handle);
        return *this;
    }

    InternetHandle &operator=(HINTERNET handle)
    {
        m_handle = handle;
        return *this;
    }

    operator HINTERNET() const
    {
        return m_handle;
    }
};

class HttpRequest
{
private:
    InternetHandle m_request;

public:
    HttpRequest(HINTERNET connection_handle, const char* path, const char* method = "GET", const std::string& body = "")
    {
        static LPCSTR accept_types[] = {"*/*", nullptr};
        // Wine hangs in HttpReadFile when handling second Keep-Alive request so HTTP/1.0 is forced
        m_request =
            HttpOpenRequest(connection_handle, method, path, "HTTP/1.0", nullptr, accept_types, INTERNET_FLAG_RELOAD, 0);
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
        if (!HttpQueryInfo(m_request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &dw_size,
                           nullptr)) {
            throw std::runtime_error("HttpQueryInfo failed");
        }

        if (status_code != 200) {
            ERR("Got status code %lu", status_code);
            throw std::runtime_error("Invalid HTTP status code");
        }
    }

    size_t read(void* buf, size_t buf_size)
    {
        DWORD bytes_read;
        TRACE("Reading HTTP response");
        if (!InternetReadFile(m_request, buf, buf_size, &bytes_read)) {
            return 0;
        }
        return bytes_read;
    }
};

class HttpConnection
{
private:
    InternetHandle m_internet;
    InternetHandle m_connection;

public:
    HttpConnection(const char* host, int port = INTERNET_DEFAULT_HTTP_PORT, const char* agent_name = nullptr)
    {
        m_internet = InternetOpen(agent_name, 0, nullptr, nullptr, 0);
        if (!m_internet) {
            throw std::runtime_error("InternetOpen failed");
        }

        DWORD timeout = 5 * 1000; // 2 seconds
        InternetSetOption(m_internet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOption(m_internet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        InternetSetOption(m_internet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        m_connection = InternetConnect(m_internet, host, port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
        if (!m_connection) {
            throw std::runtime_error("InternetConnect failed");
        }
    }

    HttpRequest request(const char* path, const char* method = "GET", const std::string& body = "")
    {
        return HttpRequest{m_connection, path, method, body};
    }

    HttpRequest get(const char* path)
    {
        return request(path, "GET");
    }

    HttpRequest post(const char* path, const std::string& body)
    {
        return request(path, "POST", body);
    }
};
