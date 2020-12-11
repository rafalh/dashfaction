#pragma once

#include <string>
#include <windows.h>

std::string get_os_version();
std::string get_real_os_version();
bool is_current_user_admin();
const char* get_process_elevation_type();
std::string get_cpu_id();
std::string get_cpu_brand();
std::string get_module_dir(HMODULE module);
