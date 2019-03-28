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

    StringMatcher(bool CaseSensitive = false) : m_CaseSensitive(CaseSensitive) {}

    StringMatcher &Exact(const std::string &Exact)
    {
        this->m_Exact = Exact;
        return *this;
    }

    StringMatcher &Prefix(const std::string &Prefix)
    {
        this->m_Prefix = Prefix;
        return *this;
    }

    StringMatcher &Infix(const std::string &Infix)
    {
        this->m_Infix = Infix;
        return *this;
    }

    StringMatcher &Suffix(const std::string &Suffix)
    {
        this->m_Suffix = Suffix;
        return *this;
    }

    bool operator()(const char *Input) const
    {
        size_t InputLen = strlen(Input);
        if (m_CaseSensitive)
        {
            if (!m_Exact.empty() && strcmp(Input, m_Exact.c_str()) != 0)
                return false;
            if (!m_Prefix.empty() && (InputLen < m_Prefix.size() || strncmp(Input, m_Prefix.c_str(), m_Prefix.size()) != 0))
                return false;
            if (!m_Infix.empty() && (InputLen < m_Infix.size() || !strstr(Input, m_Infix.c_str())))
                return false;
            if (!m_Suffix.empty() && (InputLen < m_Suffix.size() || strncmp(Input + InputLen - m_Suffix.size(), m_Suffix.c_str(), m_Suffix.size()) != 0))
                return false;
        }
        else
        {
            if (!m_Exact.empty() && stricmp(Input, m_Exact.c_str()) != 0)
                return false;
            if (!m_Prefix.empty() && (InputLen < m_Prefix.size() || strnicmp(Input, m_Prefix.c_str(), m_Prefix.size()) != 0))
                return false;
            if (!m_Infix.empty() && (InputLen < m_Infix.size() || !stristr(Input, m_Infix.c_str())))
                return false;
            if (!m_Suffix.empty() && (InputLen < m_Suffix.size() || strnicmp(Input + InputLen - m_Suffix.size(), m_Suffix.c_str(), m_Suffix.size()) != 0))
                return false;
        }
        return true;
    }
};
