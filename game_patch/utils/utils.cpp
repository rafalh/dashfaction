#include "utils.h"
#include "../rf.h"
#include <iomanip>
#include <map>
#include <common/Exception.h>
#include <common/Win32Error.h>

#ifdef __GNUC__
#ifndef __cpuid
#include <cpuid.h>
#endif
#else
#include <intrin.h>
#endif

const char* stristr(const char* haystack, const char* needle)
{
    unsigned i, j;

    for (i = 0; haystack[i]; ++i) {
        for (j = 0; needle[j]; ++j)
            if (tolower(haystack[i + j]) != tolower(needle[j]))
                break;

        if (!needle[j])
            return haystack + i;
    }
    return nullptr;
}

#define DEFINE_HRESULT_ERROR(hr) {hr, #hr},

// clang-format off
 const static std::map<HRESULT, const char*> DX_ERRORS = {
    DEFINE_HRESULT_ERROR(D3D_OK)
    DEFINE_HRESULT_ERROR(D3DERR_WRONGTEXTUREFORMAT)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDCOLOROPERATION)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDCOLORARG)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDALPHAOPERATION)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDALPHAARG)
    DEFINE_HRESULT_ERROR(D3DERR_TOOMANYOPERATIONS)
    DEFINE_HRESULT_ERROR(D3DERR_CONFLICTINGTEXTUREFILTER)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDFACTORVALUE)
    DEFINE_HRESULT_ERROR(D3DERR_CONFLICTINGRENDERSTATE)
    DEFINE_HRESULT_ERROR(D3DERR_UNSUPPORTEDTEXTUREFILTER)
    DEFINE_HRESULT_ERROR(D3DERR_CONFLICTINGTEXTUREPALETTE)
    DEFINE_HRESULT_ERROR(D3DERR_DRIVERINTERNALERROR)

    DEFINE_HRESULT_ERROR(D3DERR_NOTFOUND)
    DEFINE_HRESULT_ERROR(D3DERR_MOREDATA)
    DEFINE_HRESULT_ERROR(D3DERR_DEVICELOST)
    DEFINE_HRESULT_ERROR(D3DERR_DEVICENOTRESET)
    DEFINE_HRESULT_ERROR(D3DERR_NOTAVAILABLE)
    DEFINE_HRESULT_ERROR(D3DERR_OUTOFVIDEOMEMORY)
    DEFINE_HRESULT_ERROR(D3DERR_INVALIDDEVICE)
    DEFINE_HRESULT_ERROR(D3DERR_INVALIDCALL)
    DEFINE_HRESULT_ERROR(D3DERR_DRIVERINVALIDCALL)

    DEFINE_HRESULT_ERROR(E_OUTOFMEMORY)
};
// clang-format on

const char* getDxErrorStr(HRESULT hr)
{
    auto it = DX_ERRORS.find(hr);
    if (it != DX_ERRORS.end())
        return it->second;
    return nullptr;
}

std::string getOsVersion()
{
    OSVERSIONINFO ver_info;
    ver_info.dwOSVersionInfoSize = sizeof(ver_info);
    if (!GetVersionEx(&ver_info))
        THROW_WIN32_ERROR("GetVersionEx failed");

    return StringFormat("%lu.%lu.%lu", ver_info.dwMajorVersion, ver_info.dwMinorVersion, ver_info.dwBuildNumber);
}

std::string getRealOsVersion()
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

    return StringFormat("%d.%d.%d",                            //
                        HIWORD(file_info->dwProductVersionMS), //
                        LOWORD(file_info->dwProductVersionMS), //
                        HIWORD(file_info->dwProductVersionLS));
}

bool IsUserAdmin()
{
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    PSID administrators_group;
    BOOL ret = AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0,
                                        0, 0, 0, &administrators_group);
    if (!ret) {
        ERR("AllocateAndInitializeSid failed");
        return false;
    }

    BOOL is_member;
    if (!CheckTokenMembership(nullptr, administrators_group, &is_member)) {
        ERR("CheckTokenMembership failed");
        is_member = false;
    }
    FreeSid(administrators_group);

    return is_member != 0;
}

const char* GetProcessElevationType()
{
    HANDLE token_handle;
    TOKEN_ELEVATION_TYPE elevation_type;
    DWORD dw_size;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token_handle)) {
        ERR("OpenProcessToken failed");
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

std::string getCpuId()
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

std::string getCpuBrand()
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
            memcpy(brand_str, cpu_info, sizeof(cpu_info));
        else if (i == 0x80000003)
            memcpy(brand_str + 16, cpu_info, sizeof(cpu_info));
        else if (i == 0x80000004)
            memcpy(brand_str + 32, cpu_info, sizeof(cpu_info));
    }

    // string includes manufacturer, model and clockspeed
    return brand_str;
}
