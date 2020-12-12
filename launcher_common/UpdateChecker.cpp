#include "UpdateChecker.h"
#include <common/Exception.h>
#include <common/version.h>
#include <common/HttpRequest.h>
#include <common/utils/os-utils.h>
#include <thread>
#include <mutex>
#include <optional>

#define UPDATE_CHECK_ENDPOINT_URL "https://ravin.tk/api/rf/dashfaction/checkupdate.php"

UpdateChecker::CheckResult UpdateChecker::check()
{
    HttpSession session{"DashFaction"};
    std::string url = UPDATE_CHECK_ENDPOINT_URL "?version=" VERSION_STR;

    auto os_ver = get_real_os_version();
    url += "&os=";
    url += encode_uri_component(os_ver);

    auto wine_ver_opt = get_wine_version();
    if (wine_ver_opt) {
        url += "&wine=";
        url += encode_uri_component(wine_ver_opt.value());
    }

    HttpRequest req{url, "GET", session};
    req.send();

    char buf[4096];
    size_t bytes_read = req.read(buf, sizeof(buf) - 1);
    std::string_view response{buf, bytes_read};

    UpdateChecker::CheckResult result;
    if (!response.empty()) {
        auto new_line_pos = response.find('\n');
        if (new_line_pos != std::string_view::npos) {
            result.url = response.substr(0, new_line_pos);
            result.message = response.substr(new_line_pos + 1);
        }
        else {
            throw std::runtime_error{"Invalid response format"};
        }
    }

    return result;
}

struct AsyncUpdateChecker::SharedData
{
    std::mutex mutex;
    bool aborted = false;
    std::string error;
    std::optional<UpdateChecker::CheckResult> result_opt;
};

AsyncUpdateChecker::~AsyncUpdateChecker()
{
    if (m_shared_data) {
        std::lock_guard lg{m_shared_data->mutex};
        m_shared_data->aborted = true;
    }
}

void AsyncUpdateChecker::check_async(std::function<void()> callback)
{
    auto shared_data = std::make_shared<SharedData>();
    std::thread th([shared_data, callback]() {
        UpdateChecker checker;
        bool aborted;
        try {
            auto result = checker.check();
            std::lock_guard lg{shared_data->mutex};
            shared_data->result_opt = std::optional{result};
            aborted = shared_data->aborted;
        }
        catch (const std::exception& e) {
            std::lock_guard lg{shared_data->mutex};
            shared_data->error = e.what();
            aborted = shared_data->aborted;
        }
        if (!aborted) {
            callback();
        }
    });
    th.detach();
    m_shared_data = shared_data;
}

UpdateChecker::CheckResult AsyncUpdateChecker::get_result()
{
    std::lock_guard lg{m_shared_data->mutex};
    if (!m_shared_data->error.empty()) {
        throw std::runtime_error{m_shared_data->error};
    }
    return m_shared_data->result_opt.value();
}
