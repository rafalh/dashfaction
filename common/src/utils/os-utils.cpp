#include <common/utils/os-utils.h>
#include <xlog/xlog.h>
#include <common/error/Exception.h>
#include <common/error/Win32Error.h>
#include <cstring>
#include <format>

#ifdef __GNUC__
#ifndef __cpuid
#include <cpuid.h>
#endif
#else
#include <intrin.h>
#endif

std::string get_os_version()
{
    OSVERSIONINFO ver_info;
    ver_info.dwOSVersionInfoSize = sizeof(ver_info);
    if (!GetVersionEx(&ver_info))
        THROW_WIN32_ERROR("GetVersionEx failed");

    return std::format("{}.{}.{}", ver_info.dwMajorVersion, ver_info.dwMinorVersion, ver_info.dwBuildNumber);
}

std::string get_real_os_version()
{
    // Note: GetVersionEx is deprecated and returns values limited by application manifest from Win 8.1
    // also GetVersionEx returns compatiblity version

    char path[MAX_PATH];
    int count = GetSystemDirectory(path, std::size(path));
    if (!count)
        THROW_WIN32_ERROR("GetSystemDirectory failed");

    strcpy(path + count, "\\kernel32.dll");
    DWORD ver_size = GetFileVersionInfoSize(path, nullptr);
    if (ver_size == 0)
        THROW_WIN32_ERROR("GetFileVersionInfoSize failed");

    auto ver = std::make_unique<BYTE[]>(ver_size);
    if (!GetFileVersionInfo(path, 0, ver_size, ver.get()))
        THROW_WIN32_ERROR("GetFileVersionInfo failed");

    void* block;
    UINT block_size;
    BOOL ret = VerQueryValueA(ver.get(), "\\", &block, &block_size);
    if (!ret || block_size < sizeof(VS_FIXEDFILEINFO))
        THROW_WIN32_ERROR("VerQueryValueA returned unknown block");
    auto* file_info = static_cast<VS_FIXEDFILEINFO*>(block);

    return std::format("{}.{}.{}",
                        HIWORD(file_info->dwProductVersionMS),
                        LOWORD(file_info->dwProductVersionMS),
                        HIWORD(file_info->dwProductVersionLS));
}

std::optional<std::string> get_wine_version()
{
    HMODULE ntdll_handle = GetModuleHandleA("ntdll.dll");
    // Note: double cast is needed to fix cast-function-type GCC warning
    auto* wine_get_version = reinterpret_cast<const char*(*)()>(reinterpret_cast<void(*)()>(
        GetProcAddress(ntdll_handle, "wine_get_version")));
    if (!wine_get_version)
        return {};
    const char* ver = wine_get_version();
    return {ver};
}

bool is_current_user_admin()
{
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    PSID administrators_group;
    BOOL ret = AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
                                        0, 0, 0, &administrators_group);
    if (!ret) {
        xlog::error("AllocateAndInitializeSid failed");
        return false;
    }

    BOOL is_member;
    if (!CheckTokenMembership(nullptr, administrators_group, &is_member)) {
        xlog::error("CheckTokenMembership failed");
        is_member = false;
    }
    FreeSid(administrators_group);

    return is_member != 0;
}

const char* get_process_elevation_type()
{
    HANDLE token_handle;
    TOKEN_ELEVATION_TYPE elevation_type;
    DWORD dw_size;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle)) {
        xlog::error("OpenProcessToken failed");
        return "unknown";
    }
    if (!GetTokenInformation(token_handle, TokenElevationType, &elevation_type, sizeof(elevation_type), &dw_size))
        elevation_type = TokenElevationTypeDefault;

    CloseHandle(token_handle);

    switch (elevation_type) {
    case TokenElevationTypeDefault:
        return "default";
    case TokenElevationTypeFull:
        return "full";
    case TokenElevationTypeLimited:
        return "limited";
    default:
        return "unknown";
    }
}

std::string get_cpu_id()
{
    int cpu_info[4] = {0};
#ifndef __GNUC__
    __cpuid(cpu_info, 1);
#else
    __cpuid(1, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
#endif
    return std::format("{:08X} {:08X} {:08X} {:08X}",
        static_cast<unsigned>(cpu_info[0]),
        static_cast<unsigned>(cpu_info[1]),
        static_cast<unsigned>(cpu_info[2]),
        static_cast<unsigned>(cpu_info[3]));
}

std::string get_cpu_brand()
{
    int cpu_info[4] = {0};
    char brand_str[0x40] = "";
    // Get the information associated with each extended ID.
#ifndef __GNUC__
    __cpuid(cpu_info, 0x80000000);
#else
    __cpuid(0x80000000, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
#endif
    unsigned max_ext_id = cpu_info[0];
    for (unsigned i = 0x80000000; i <= max_ext_id; ++i) {
#ifndef __GNUC__
        __cpuid(cpu_info, i);
#else
        __cpuid(i, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
#endif
        // Interpret CPU brand string
        if (i == 0x80000002)
            std::memcpy(brand_str, cpu_info, sizeof(cpu_info));
        else if (i == 0x80000003)
            std::memcpy(brand_str + 16, cpu_info, sizeof(cpu_info));
        else if (i == 0x80000004)
            std::memcpy(brand_str + 32, cpu_info, sizeof(cpu_info));
    }

    // string includes manufacturer, model and clockspeed
    return brand_str;
}

std::string get_module_pathname(HMODULE module)
{
    constexpr int max_attempts = 5;
    std::string buf(MAX_PATH, '\0');
    for (int i = 0; i < max_attempts; ++i) {
        auto num_copied = GetModuleFileNameA(module, buf.data(), buf.size());
        if (num_copied == 0) {
            xlog::error("GetModuleFileNameA failed ({})", GetLastError());
            return {};
        }
        if (num_copied < buf.size()) {
            buf.resize(num_copied);
            return buf;
        }
        buf.resize(buf.size() * 2);
    }
    xlog::error("GetModuleFileNameA is misbehaving or pathname is longer than {}...", buf.size());
    return {};
}

std::string get_module_dir(HMODULE module)
{
    auto pathname = get_module_pathname(module);
    auto last_sep = pathname.rfind('\\');
    if (last_sep != std::string::npos) {
        pathname.resize(last_sep + 1);
    }
    return pathname;
}

std::string get_temp_path_name(const char* prefix)
{
    char temp_dir[MAX_PATH];
    DWORD ret_val = GetTempPathA(std::size(temp_dir), temp_dir);
    if (ret_val == 0 || ret_val > std::size(temp_dir)) {
        throw std::runtime_error("GetTempPathA failed");
    }

    char result[MAX_PATH];
    ret_val = GetTempFileNameA(temp_dir, prefix, 0, result);
    if (ret_val == 0) {
        throw std::runtime_error("GetTempFileNameA failed");
    }

    return result;
}
