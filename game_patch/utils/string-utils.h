#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <memory>
#include <cstdarg>
#include <cstring>

#ifdef __GNUC__
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx) __attribute__ ((format (printf, fmt_idx, va_idx)))
#else
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx)
#endif

inline const char* stristr(const char* haystack, const char* needle)
{
    for (size_t i = 0; haystack[i]; ++i) {
        size_t j;
        for (j = 0; needle[j]; ++j) {
            if (tolower(haystack[i + j]) != tolower(needle[j])) {
                break;
            }
        }

        if (!needle[j]) {
            return haystack + i;
        }
    }
    return nullptr;
}

inline std::vector<std::string_view> StringSplit(std::string_view str, std::string_view delims = " ")
{
    std::vector<std::string_view> output;
    size_t first = 0;

    while (first < str.size()) {
        const auto second = str.find_first_of(delims, first);

        if (first != second)
            output.emplace_back(str.substr(first, second - first));

        if (second == std::string_view::npos)
            break;

        first = second + 1;
    }

    return output;
}

inline std::string StringToLower(std::string_view str)
{
    std::string output;
    output.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(output), ::tolower);
    return output;
}

struct StringMatcher
{
private:
    std::string m_exact, m_prefix, m_infix, m_suffix;
    bool m_case_sensitive;

public:
    StringMatcher(bool case_sensitive = false) : m_case_sensitive(case_sensitive) {}

    StringMatcher& Exact(const std::string& exact)
    {
        this->m_exact = exact;
        return *this;
    }

    StringMatcher& Prefix(const std::string& prefix)
    {
        this->m_prefix = prefix;
        return *this;
    }

    StringMatcher& Infix(const std::string& infix)
    {
        this->m_infix = infix;
        return *this;
    }

    StringMatcher& Suffix(const std::string& suffix)
    {
        this->m_suffix = suffix;
        return *this;
    }

    bool operator()(const char* input) const
    {
        size_t input_len = std::strlen(input);
        if (m_case_sensitive) {
            if (!m_exact.empty() && std::strcmp(input, m_exact.c_str()) != 0)
                return false;
            if (!m_prefix.empty() &&
                (input_len < m_prefix.size() || std::strncmp(input, m_prefix.c_str(), m_prefix.size()) != 0))
                return false;
            if (!m_infix.empty() && (input_len < m_infix.size() || !std::strstr(input, m_infix.c_str())))
                return false;
            if (!m_suffix.empty() && (input_len < m_suffix.size() || std::strncmp(input + input_len - m_suffix.size(),
                                                                             m_suffix.c_str(), m_suffix.size()) != 0))
                return false;
        }
        else {
            if (!m_exact.empty() && stricmp(input, m_exact.c_str()) != 0)
                return false;
            if (!m_prefix.empty() &&
                (input_len < m_prefix.size() || strnicmp(input, m_prefix.c_str(), m_prefix.size()) != 0))
                return false;
            if (!m_infix.empty() && (input_len < m_infix.size() || !stristr(input, m_infix.c_str())))
                return false;
            if (!m_suffix.empty() && (input_len < m_suffix.size() || strnicmp(input + input_len - m_suffix.size(),
                                                                              m_suffix.c_str(), m_suffix.size()) != 0))
                return false;
        }
        return true;
    }
};

PRINTF_FMT_ATTRIBUTE(1, 2)
inline std::string StringFormat(const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    int size = vsnprintf(nullptr, 0, format, args) + 1;// Extra space for '\0'
    va_end(args);
    std::unique_ptr<char[]> buf(new char[size]);
    va_start(args, format);
    vsnprintf(buf.get(), size, format, args);
    va_end(args);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

inline std::string_view GetFilenameWithoutExt(std::string_view filename)
{
    auto dot_pos = filename.rfind('.');
    if (dot_pos == std::string_view::npos) {
        return filename;
    }
    return filename.substr(0, dot_pos);
}
