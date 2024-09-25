#include "dashoptions.h"
#include "../rf/file/file.h"
#include <algorithm>
#include <sstream>
#include <cctype>
#include <string>
#include <xlog/xlog.h>

namespace dashopt
{
// Encapsulate file handling within a function-local static
rf::File& get_dashoptions_file()
{
    static rf::File dashoptions_file;
    return dashoptions_file;
}

// Helper function to trim leading and trailing whitespace
std::string trim(const std::string& str)
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}


// Encapsulate option handlers within a safe function-local static
std::map<std::string, std::pair<OptionType, std::function<void(const ConfigOption&)>>>& get_option_handlers()
{
    static std::map<std::string, std::pair<OptionType, std::function<void(const ConfigOption&)>>> option_handlers;
    return option_handlers;
}

bool open_file(const std::string& file_path)
{
    auto& dashoptions_file = get_dashoptions_file(); // Use the encapsulated file instance
    if (dashoptions_file.open(file_path.c_str()) != 0) {
        xlog::error("Failed to open {}", file_path);
        return false;
    }
    xlog::warn("Successfully opened {}", file_path);
    return true;
}

void close_file()
{
    auto& dashoptions_file = get_dashoptions_file(); // Use the encapsulated file instance
    dashoptions_file.close();
}

void add_option_handler(const std::string& option_name, OptionType type,
                        std::function<void(const ConfigOption&)> handler)
{
    get_option_handlers()[option_name] = std::make_pair(type, handler);
}

void parse()
{
    std::string line;
    char buffer[256];
    auto& dashoptions_file = get_dashoptions_file(); // Use the encapsulated file instance

    while (int bytes_read = dashoptions_file.read(buffer, sizeof(buffer) - 1)) {
        if (bytes_read <= 0)
            break;

        buffer[bytes_read] = '\0';
        line = std::string(buffer);
        line = trim(line);

        auto delimiter_pos = line.find(':');
        if (delimiter_pos == std::string::npos)
            continue;

        std::string option_name = trim(line.substr(0, delimiter_pos));
        std::string option_value = trim(line.substr(delimiter_pos + 1));

        auto& option_handlers = get_option_handlers();
        if (option_handlers.find(option_name) != option_handlers.end()) {
            auto [expected_type, handler] = option_handlers[option_name];
            ConfigOption option;
            option.type = expected_type;

            // Parse based on the expected type
            switch (expected_type) {
            case OptionType::String:
                if (option_value.front() == '"' && option_value.back() == '"') {
                    option.string_value = option_value.substr(1, option_value.size() - 2);
                }
                break;
            case OptionType::Float:
                try {
                    option.float_value = std::stof(option_value);
                }
                catch (...) {
                    xlog::warn("Invalid float for option: {}", option_name);
                }
                break;
            case OptionType::Integer:
                try {
                    option.int_value = std::stoi(option_value);
                }
                catch (...) {
                    xlog::warn("Invalid integer for option: {}", option_name);
                }
                break;
            }

            // Call the handler with the parsed option
            handler(option);
        }
        else {
            xlog::warn("Unknown option: {}", option_name);
        }
    }
}

// Function to load dashoptions configuration
void load_dashoptions_config()
{
    // Open the file using the game's file system
    if (!open_file("dashoptions.tbl")) {
        return;
    }

    // Register handlers for each expected option
    add_option_handler("$Print", OptionType::String, [](const ConfigOption& option) {
        if (option.string_value) {
            xlog::warn("Dash Options - Print: {}", option.string_value.value());
        }
    });

    add_option_handler("$Player Walk Speed", OptionType::Float, [](const ConfigOption& option) {
        if (option.float_value) {
            xlog::warn("Player Walk Speed set to {}", option.float_value.value());
            // Set player walk speed in your system here
        }
    });

    add_option_handler("$Num Shotgun Projectiles", OptionType::Integer, [](const ConfigOption& option) {
        if (option.int_value) {
            xlog::warn("Num Shotgun Projectiles set to {}", option.int_value.value());
            // Set shotgun projectiles in your system here
        }
    });

    // Parse the file and handle the options
    parse();

    // Close the file when done
    close_file();
}

} // namespace dashopt
