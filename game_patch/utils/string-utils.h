#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdarg>
#include <cctype>

#ifdef __GNUC__
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx) __attribute__ ((format (printf, fmt_idx, va_idx)))
#else
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx)
#endif

inline std::vector<std::string_view> StringSplit(std::string_view str, char delim = ' ')
{
    std::vector<std::string_view> output;
    size_t first = 0;

    while (first < str.size()) {
        const auto second = str.find_first_of(delim, first);

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

inline bool StringEqualsIgnoreCase(std::string_view left, std::string_view right)
{
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](auto a, auto b) {
        return std::tolower(a) == std::tolower(b);
    });
}

inline bool StringStartsWith(std::string_view str, std::string_view prefix)
{
    return str.substr(0, prefix.size()) == prefix;
}

inline bool StringStartsWithIgnoreCase(std::string_view str, std::string_view prefix)
{
    return StringEqualsIgnoreCase(str.substr(0, prefix.size()), prefix);
}

inline bool StringEndsWith(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

inline bool StringEndsWithIgnoreCase(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && StringEqualsIgnoreCase(str.substr(str.size() - suffix.size()), suffix);
}

inline bool StringContains(std::string_view str, char ch)
{
    return str.find(ch) != std::string_view::npos;
}

inline bool StringContains(std::string_view str, std::string_view infix)
{
    return str.find(infix) != std::string_view::npos;
}

inline bool StringContainsIgnoreCase(std::string_view str, std::string_view infix)
{
    auto it = std::search(str.begin(), str.end(),
        infix.begin(), infix.end(),  [](auto a, auto b) {
        return std::tolower(a) == std::tolower(b);
    });
    return it != str.end();
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

    bool operator()(std::string_view input) const
    {
        if (m_case_sensitive) {
            if (!m_exact.empty() && input != m_exact)
                return false;
            if (!m_prefix.empty() && !StringStartsWith(input, m_prefix))
                return false;
            if (!m_infix.empty() && !StringContains(input, m_infix))
                return false;
            if (!m_suffix.empty() && !StringEndsWith(input, m_suffix))
                return false;
        }
        else {
            if (!m_exact.empty() && !StringEqualsIgnoreCase(input, m_exact))
                return false;
            if (!m_prefix.empty() && !StringStartsWithIgnoreCase(input, m_prefix))
                return false;
            if (!m_infix.empty() && !StringContainsIgnoreCase(input, m_infix))
                return false;
            if (!m_suffix.empty() && !StringEndsWithIgnoreCase(input, m_suffix))
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
    int size = vsnprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
    va_end(args);
    std::string str;
    str.resize(size);
    va_start(args, format);
    vsnprintf(str.data(), size, format, args);
    va_end(args);
    str.resize(size - 1); // We don't want the '\0' inside
    return str;
}

inline std::string_view GetFilenameWithoutExt(std::string_view filename)
{
    auto dot_pos = filename.rfind('.');
    if (dot_pos == std::string_view::npos) {
        return filename;
    }
    return filename.substr(0, dot_pos);
}

inline std::string_view GetExtFromFileName(std::string_view filename)
{
    auto dot_pos = filename.rfind('.');
    if (dot_pos == std::string_view::npos) {
        return "";
    }
    return filename.substr(dot_pos + 1);
}

