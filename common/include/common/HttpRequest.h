#pragma once

#include <string>
#include <utility>
#include <optional>
#include <windows.h>
#include <wininet.h>

class InternetHandle
{
private:
    HINTERNET m_handle = nullptr;

public:
    InternetHandle() = default;

    ~InternetHandle()
    {
        if (m_handle)
            InternetCloseHandle(m_handle);
    }

    InternetHandle(const InternetHandle&) = delete;            // copy constructor
    InternetHandle& operator=(const InternetHandle&) = delete; // assignment operator

    InternetHandle(InternetHandle&& other) noexcept : // move constructor
        m_handle(std::exchange(other.m_handle, nullptr))
    {}

    InternetHandle& operator=(InternetHandle&& other) noexcept // move assignment
    {
        std::swap(m_handle, other.m_handle);
        return *this;
    }

    InternetHandle& operator=(HINTERNET handle)
    {
        if (m_handle)
            InternetCloseHandle(m_handle);
        m_handle = handle;
        return *this;
    }

    operator HINTERNET() const
    {
        return m_handle;
    }
};

class HttpSession
{
private:
    InternetHandle m_inet;

public:
    HttpSession(const char* user_agent);

    void set_connect_timeout(unsigned long timeout_ms);
    void set_send_timeout(unsigned long timeout_ms);
    void set_receive_timeout(unsigned long timeout_ms);

    HINTERNET get_internet_handle()
    {
        return m_inet;
    }
};

class HttpRequest
{
private:
    InternetHandle m_conn;
    InternetHandle m_req;
    bool m_in_body = false;

public:
    HttpRequest(std::string_view url, const char* method, HttpSession& session);
    void add_header(std::string_view name, std::string_view value);
    void add_raw_headers(std::string_view headers);

    void set_content_type(std::string_view content_type)
    {
        add_header("Content-Type", content_type);
    }

    void begin_body(size_t total_body_size = 0);
    void write(const void* data, size_t len);

    void write(std::string_view data)
    {
        write(data.data(), data.size());
    }

    void send(std::string_view body = "");
    size_t read(void* buf, size_t buf_size);
    std::optional<std::string> get_header(std::string_view name);
};

std::string encode_uri_component(std::string_view value);
