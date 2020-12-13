#include <common/utils/os-utils.h>
#include <common/utils/string-utils.h>
#include <xlog/xlog.h>
#include <iomanip>
#include <common/error/Exception.h>
#include <common/error/Win32Error.h>
#include <cstring>

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

    return string_format("%lu.%lu.%lu", ver_info.dwMajorVersion, ver_info.dwMinorVersion, ver_info.dwBuildNumber);
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
    VS_FIXEDFILEINFO* file_info = reinterpret_cast<VS_FIXEDFILEINFO*>(block);

    return string_format("%d.%d.%d",                            //
                        HIWORD(file_info->dwProductVersionMS), //
                        LOWORD(file_info->dwProductVersionMS), //
                        HIWORD(file_info->dwProductVersionLS));
}

std::optional<std::string> get_wine_version()
{
    auto ntdll_handle = GetModuleHandleA("ntdll.dll");
    // Note: double cast is needed to fix cast-function-type GCC warning
    auto wine_get_version = reinterpret_cast<const char*(*)()>(reinterpret_cast<void(*)()>(
        GetProcAddress(ntdll_handle, "wine_get_version")));
    if (!wine_get_version)
        return {};
    auto ver = wine_get_version();
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
    std::stringstream ss;
#ifndef __GNUC__
    __cpuid(cpu_info, 1);
#else
    __cpuid(1, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
#endif
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << cpu_info[0] << ' ' << cpu_info[1] << ' '
       << cpu_info[2] << ' ' << cpu_info[3];
    return ss.str();
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

std::string get_module_dir(HMODULE module)
{
    std::string buf(MAX_PATH, '\0');
    auto num_copied = GetModuleFileNameA(module, buf.data(), buf.size());
    if (num_copied == buf.size()) {
        xlog::error("GetModuleFileNameA failed (%lu)", GetLastError());
        return {};
    }
    buf.resize(num_copied);
    auto last_sep = buf.rfind('\\');
    if (last_sep != std::string::npos) {
        buf.resize(last_sep + 1);
    }
    return buf;
}
