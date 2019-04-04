#pragma once

#include <log/Logger.h>
#include <windef.h>

const char* stristr(const char* haystack, const char* needle);

const char* getDxErrorStr(HRESULT hr);
std::string getOsVersion();
std::string getRealOsVersion();
bool IsUserAdmin();
const char* GetProcessElevationType();
std::string getCpuId();
std::string getCpuBrand();

#define STDCALL __stdcall

#define ASSERT(x) assert(x)

struct StringMatcher
{
    std::string m_Exact, m_Prefix, m_Infix, m_Suffix;
    bool m_CaseSensitive;

    StringMatcher(bool case_sensitive = false) : m_CaseSensitive(case_sensitive) {}

    StringMatcher& Exact(const std::string& exact)
    {
        this->m_Exact = exact;
        return *this;
    }

    StringMatcher& Prefix(const std::string& prefix)
    {
        this->m_Prefix = prefix;
        return *this;
    }

    StringMatcher& Infix(const std::string& infix)
    {
        this->m_Infix = infix;
        return *this;
    }

    StringMatcher& Suffix(const std::string& suffix)
    {
        this->m_Suffix = suffix;
        return *this;
    }

    bool operator()(const char* input) const
    {
        size_t input_len = strlen(input);
        if (m_CaseSensitive) {
            if (!m_Exact.empty() && strcmp(input, m_Exact.c_str()) != 0)
                return false;
            if (!m_Prefix.empty() &&
                (input_len < m_Prefix.size() || strncmp(input, m_Prefix.c_str(), m_Prefix.size()) != 0))
                return false;
            if (!m_Infix.empty() && (input_len < m_Infix.size() || !strstr(input, m_Infix.c_str())))
                return false;
            if (!m_Suffix.empty() && (input_len < m_Suffix.size() || strncmp(input + input_len - m_Suffix.size(),
                                                                             m_Suffix.c_str(), m_Suffix.size()) != 0))
                return false;
        }
        else {
            if (!m_Exact.empty() && stricmp(input, m_Exact.c_str()) != 0)
                return false;
            if (!m_Prefix.empty() &&
                (input_len < m_Prefix.size() || strnicmp(input, m_Prefix.c_str(), m_Prefix.size()) != 0))
                return false;
            if (!m_Infix.empty() && (input_len < m_Infix.size() || !stristr(input, m_Infix.c_str())))
                return false;
            if (!m_Suffix.empty() && (input_len < m_Suffix.size() || strnicmp(input + input_len - m_Suffix.size(),
                                                                              m_Suffix.c_str(), m_Suffix.size()) != 0))
                return false;
        }
        return true;
    }
};

template<typename... Args>
std::string StringFormat(const char* format, Args... args)
{
    size_t size = snprintf(nullptr, 0, format, args...) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format, args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
