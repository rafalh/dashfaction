#pragma once

#include <functional>
#include <memory>
#include <string>

class UpdateChecker
{
public:
    struct CheckResult
    {
        std::string message;
        std::string url;

        operator bool() const
        {
            return !message.empty();
        }
    };

    CheckResult check();
};

class AsyncUpdateChecker
{
    struct SharedData;
    std::shared_ptr<SharedData> m_shared_data;

public:
    ~AsyncUpdateChecker();
    void check_async(std::function<void()> callback);
    UpdateChecker::CheckResult get_result();
};
