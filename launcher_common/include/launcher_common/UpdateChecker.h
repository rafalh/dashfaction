#pragma once

#include <functional>
#include <thread>
#include <windows.h>

class UpdateChecker
{
public:
    UpdateChecker(HWND hwnd = nullptr) : m_hwnd(hwnd) {}

    bool check();
    void check_async(std::function<void()> callback);
    void abort();

    bool is_new_version_available() const
    {
        return !m_message.empty();
    }

    const std::string& get_message() const
    {
        return m_message;
    }

    const std::string& get_url() const
    {
        return m_url;
    }

    bool has_error() const
    {
        return !m_error.empty();
    }

    const std::string& get_error() const
    {
        return m_error;
    }

private:
    void thread_proc();

    std::thread m_thread;
    HWND m_hwnd;
    bool m_abort = false;
    bool m_new_version = false;
    std::string m_message;
    std::string m_url;
    std::string m_error;
    std::function<void()> m_callback;
};
