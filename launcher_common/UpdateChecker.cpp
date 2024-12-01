#include "UpdateChecker.h"
#include <common/error/Exception.h>
#include <common/version/version.h>
#include <common/HttpRequest.h>
#include <common/utils/os-utils.h>
#include <thread>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <ed25519.h>
#include <base64.h>

#define UPDATE_CHECK_ENDPOINT_URL "https://dashfactionapi.rafalh.dev/update"

static unsigned char signature_public_key[] = {
    0x44, 0xCB, 0x46, 0xDC, 0xD6, 0xC7, 0xDB, 0xA0, 0x49, 0x60, 0xB5, 0x42, 0x1E, 0x14, 0xA1, 0xBD,
    0x68, 0x3D, 0xC3, 0xE7, 0x29, 0x67, 0x17, 0xE3, 0x2F, 0xAD, 0xA1, 0x71, 0x33, 0x13, 0x4C, 0x7C,
};

static bool verify_signature(
    const std::optional<std::string>& sig_hdr_opt, std::string_view response_body, const std::string& challenge
)
{
    std::string sig_hdr = sig_hdr_opt.value();
    std::string sig = base64_decode(sig_hdr);
    std::string msg{response_body};
    msg += challenge;
    int result = ed25519_verify(
        reinterpret_cast<const unsigned char*>(sig.data()), reinterpret_cast<const unsigned char*>(msg.data()),
        msg.size(), signature_public_key
    );
    return result != 0;
}

UpdateChecker::CheckResult UpdateChecker::check() // NOLINT(readability-convert-member-functions-to-static)
{
    HttpSession session{"DashFaction"};
    std::string url = UPDATE_CHECK_ENDPOINT_URL "?version=" VERSION_STR;

    auto os_ver = get_real_os_version();
    url += "&os=";
    url += encode_uri_component(os_ver);
    unsigned char random_bytes[32];
    if (ed25519_create_seed(random_bytes) != 0) {
        throw std::runtime_error{"Cannot generate random seed"};
    }
    std::string challenge = base64_encode(random_bytes, 32, true);
    url += "&signature-input=";
    url += encode_uri_component(challenge);

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

    std::optional<std::string> sig_hdr_opt = req.get_header("X-Signature");
    if (!verify_signature(sig_hdr_opt, response, challenge)) {
        throw std::runtime_error{"Invalid signature"};
    }

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
