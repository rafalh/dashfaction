#pragma once

#include <functional>
#include <thread>
#include <windef.h>

class UpdateChecker
{
public:
    UpdateChecker(HWND hwnd = nullptr) : m_hwnd(hwnd) {}

    bool check();
    void checkAsync(std::function<void()> callback);
    void abort();

    bool isNewVersionAvailable() const
    {
        return !m_message.empty();
    }

    const std::string& getMessage() const
    {
        return m_message;
    }

    const std::string& getUrl() const
    {
        return m_url;
    }

    bool hasError() const
    {
        return !m_error.empty();
    }

    const std::string& getError() const
    {
        return m_error;
    }

private:
    void threadProc();

    std::thread m_thread;
    HWND m_hwnd;
    bool m_abort = false;
    bool m_newVersion = false;
    std::string m_message;
    std::string m_url;
    std::string m_error;
    std::function<void()> m_callback;
};
