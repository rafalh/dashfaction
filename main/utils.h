#pragma once

#include <windef.h>
#include <log/Logger.h>

const char *stristr(const char *haystack, const char *needle);

const char *getDxErrorStr(HRESULT hr);
std::string getOsVersion();
std::string getRealOsVersion();
bool IsUserAdmin();
const char *GetProcessElevationType();
std::string getCpuId();
std::string getCpuBrand();

#define COUNTOF(a) (sizeof(a)/sizeof((a)[0]))

#define NAKED __declspec(naked)
#define STDCALL __stdcall

#define ASSERT(x) assert(x)

template<typename T>
T clamp(T val, T min, T max)
{
    return std::min(std::max(val, min), max);
}

struct StringMatcher
{
    std::string m_Exact, m_Prefix, m_Infix, m_Suffix;
    bool m_CaseSensitive;

    StringMatcher(bool case_sensitive = false) : m_CaseSensitive(case_sensitive) {}

    StringMatcher &Exact(const std::string &exact)
    {
        this->m_Exact = exact;
        return *this;
    }

    StringMatcher &Prefix(const std::string &prefix)
    {
        this->m_Prefix = prefix;
        return *this;
    }

    StringMatcher &Infix(const std::string &infix)
    {
        this->m_Infix = infix;
        return *this;
    }

    StringMatcher &Suffix(const std::string &suffix)
    {
        this->m_Suffix = suffix;
        return *this;
    }

    bool operator()(const char *input) const
    {
        size_t InputLen = strlen(input);
        if (m_CaseSensitive)
        {
            if (!m_Exact.empty() && strcmp(input, m_Exact.c_str()) != 0)
                return false;
            if (!m_Prefix.empty() && (InputLen < m_Prefix.size() || strncmp(input, m_Prefix.c_str(), m_Prefix.size()) != 0))
                return false;
            if (!m_Infix.empty() && (InputLen < m_Infix.size() || !strstr(input, m_Infix.c_str())))
                return false;
            if (!m_Suffix.empty() && (InputLen < m_Suffix.size() || strncmp(input + InputLen - m_Suffix.size(), m_Suffix.c_str(), m_Suffix.size()) != 0))
                return false;
        }
        else
        {
            if (!m_Exact.empty() && stricmp(input, m_Exact.c_str()) != 0)
                return false;
            if (!m_Prefix.empty() && (InputLen < m_Prefix.size() || strnicmp(input, m_Prefix.c_str(), m_Prefix.size()) != 0))
                return false;
            if (!m_Infix.empty() && (InputLen < m_Infix.size() || !stristr(input, m_Infix.c_str())))
                return false;
            if (!m_Suffix.empty() && (InputLen < m_Suffix.size() || strnicmp(input + InputLen - m_Suffix.size(), m_Suffix.c_str(), m_Suffix.size()) != 0))
                return false;
        }
        return true;
    }
};
