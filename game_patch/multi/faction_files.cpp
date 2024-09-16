#include <string>
#include <optional>
#include <fstream>
#include <sstream>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <xlog/xlog.h>
#include "faction_files.h"

static const char level_download_agent_name[] = "Dash Faction";
static const char level_download_base_url[] = "https://autodl.factionfiles.com";

FactionFilesClient::FactionFilesClient() : session_{level_download_agent_name}
{
    session_.set_connect_timeout(2000);
    session_.set_receive_timeout(3000);
}

std::optional<FactionFilesClient::LevelInfo> FactionFilesClient::parse_level_info(const char* buf)
{
    std::stringstream ss(buf);
    ss.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string temp;
    std::getline(ss, temp);
    if (temp != "found") {
        if (temp != "notfound") {
            xlog::warn("Invalid FactionFiles response: %s", buf);
        }
        // not found
        return {};
    }

    LevelInfo info;
    std::getline(ss, info.name);
    std::getline(ss, info.author);
    std::getline(ss, info.description);

    std::getline(ss, temp);
    info.size_in_bytes = static_cast<unsigned>(std::stof(temp) * 1024u * 1024u);
    if (!info.size_in_bytes) {
        throw std::runtime_error("invalid file size");
    }

    std::getline(ss, temp);
    info.ticket_id = std::stoi(temp);
    if (info.ticket_id <= 0) {
        throw std::runtime_error("invalid ticket id");
    }

    return {info};
}

std::optional<FactionFilesClient::LevelInfo> FactionFilesClient::find_map(const char* file_name)
{
    auto url = std::format("{}/findmap.php?rflName={}", level_download_base_url, encode_uri_component(file_name));

    xlog::trace("Fetching level info: %s", file_name);
    HttpRequest req{url, "GET", session_};
    req.send();

    char buf[256];
    size_t num_bytes_read = req.read(buf, sizeof(buf) - 1);
    if (num_bytes_read == 0) {
        throw std::runtime_error("empty response");
    }

    buf[num_bytes_read] = '\0';
    xlog::debug("FactionFiles response: %s", buf);

    return parse_level_info(buf);
}

void FactionFilesClient::download_map(const char* tmp_filename, int ticket_id,
    std::function<bool(unsigned bytes_received, std::chrono::milliseconds duration)> callback)
{
    auto url = std::format("{}/downloadmap.php?ticketid={}", level_download_base_url, ticket_id);
    HttpRequest req{url, "GET", session_};
    req.send();

    std::ofstream tmp_file(tmp_filename, std::ios_base::out | std::ios_base::binary);
    if (!tmp_file) {
        xlog::error("Cannot open file: %s", tmp_filename);
        throw std::runtime_error("cannot open file");
    }

    // Note: we are connected here - don't include it in duration so speed calculation can be more precise
    tmp_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    auto download_start = std::chrono::steady_clock::now();
    unsigned total_bytes_read = 0;

    char buf[4096];
    while (true) {
        auto num_bytes_read = req.read(buf, sizeof(buf));
        if (num_bytes_read <= 0)
            break;

        tmp_file.write(buf, num_bytes_read);

        total_bytes_read += num_bytes_read;
        auto time_diff = std::chrono::steady_clock::now() - download_start;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(time_diff);

        if (callback && !callback(total_bytes_read, duration)) {
            xlog::debug("Download aborted");
            throw std::runtime_error("download aborted");
        }
    }
}
