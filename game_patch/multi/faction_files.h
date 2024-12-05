#pragma once

#include <string>
#include <common/HttpRequest.h>

class FactionFilesClient
{
public:
    struct LevelInfo
    {
        std::string name;
        std::string author;
        std::string description;
        unsigned size_in_bytes;
        int ticket_id;
    };

    FactionFilesClient();
    std::optional<LevelInfo> find_map(const char* file_name);
    void download_map(
        const char* tmp_filename, int ticket_id,
        std::function<bool(unsigned bytes_received, std::chrono::milliseconds duration)> callback
    );

private:
    HttpSession session_;

    static std::optional<LevelInfo> parse_level_info(const char* buf);
};
