#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>

// Enumeration for supported option types
enum class OptionType
{
    String,
    Float,
    Integer
};

// Structure to hold each configuration option with its expected type and value
struct ConfigOption
{
    OptionType type;
    std::optional<std::string> string_value;
    std::optional<float> float_value;
    std::optional<int> int_value;
};

// Namespace to encapsulate dash options parsing logic
namespace dashopt
{
// Open file function
bool open_file(const std::string& file_path);

// Close file function
void close_file();

// Parse function that iterates through the file and parses each option
void parse();

// Add option handler for each configuration option
void add_option_handler(const std::string& option_name, OptionType type,
                        std::function<void(const ConfigOption&)> handler);

// Load DashOptions configuration and register handlers
void load_dashoptions_config();

} // namespace dashopt
