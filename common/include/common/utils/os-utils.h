#pragma once

#include <string>
#include <optional>
#include <windows.h>

std::string get_os_version();
std::string get_real_os_version();
std::optional<std::string> get_wine_version();
bool is_current_user_admin();
const char* get_process_elevation_type();
std::string get_cpu_id();
std::string get_cpu_brand();
std::string get_module_pathname(HMODULE module);
std::string get_module_dir(HMODULE module);
std::string get_temp_path_name(const char* prefix);
