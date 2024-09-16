#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cctype>

#ifdef __GNUC__
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx) __attribute__ ((format (printf, fmt_idx, va_idx)))
#else
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx)
#endif

inline std::vector<std::string_view> string_split(std::string_view str, char delim = ' ')
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

inline std::string string_to_lower(std::string_view str)
{
    std::string output;
    output.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(output), [](unsigned char ch) {
        return std::tolower(ch);
    });
    return output;
}

inline bool string_equals_ignore_case(std::string_view left, std::string_view right)
{
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](unsigned char a, unsigned char b) {
        return std::tolower(a) == std::tolower(b);
    });
}

inline bool string_starts_with(std::string_view str, std::string_view prefix)
{
    return str.starts_with(prefix);
}

inline bool string_starts_with_ignore_case(std::string_view str, std::string_view prefix)
{
    return string_equals_ignore_case(str.substr(0, prefix.size()), prefix);
}

inline bool string_ends_with(std::string_view str, std::string_view suffix)
{
    return str.ends_with(suffix);
}

inline bool string_ends_with_ignore_case(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && string_equals_ignore_case(str.substr(str.size() - suffix.size()), suffix);
}

inline bool string_contains(std::string_view str, char ch)
{
    return str.find(ch) != std::string_view::npos;
}

inline bool string_contains(std::string_view str, std::string_view infix)
{
    return str.find(infix) != std::string_view::npos;
}

inline bool string_contains_ignore_case(std::string_view str, std::string_view infix)
{
    std::string_view::iterator it = std::search(str.begin(), str.end(),
        infix.begin(), infix.end(),  [](unsigned char a, unsigned char b) {
        return std::tolower(a) == std::tolower(b);
    });
    return it != str.end();
}

inline std::string string_replace(const std::string_view& str, const std::string_view& search, const std::string_view& replacement)
{
    std::size_t pos = 0;
    std::string result{str};
    while (true) {
        pos = result.find(search, pos);
        if (pos == std::string::npos) {
            break;
        }
        result.replace(pos, search.size(), replacement);
        pos += replacement.size();
    }
    return result;
}

struct StringMatcher
{
private:
    std::string m_exact, m_prefix, m_infix, m_suffix;
    bool m_case_sensitive;

public:
    StringMatcher(bool case_sensitive = false) : m_case_sensitive(case_sensitive) {}

    StringMatcher& exact(const std::string& exact)
    {
        this->m_exact = exact;
        return *this;
    }

    StringMatcher& prefix(const std::string& prefix)
    {
        this->m_prefix = prefix;
        return *this;
    }

    StringMatcher& infix(const std::string& infix)
    {
        this->m_infix = infix;
        return *this;
    }

    StringMatcher& suffix(const std::string& suffix)
    {
        this->m_suffix = suffix;
        return *this;
    }

    bool operator()(std::string_view input) const
    {
        if (m_case_sensitive) {
            if (!m_exact.empty() && input != m_exact)
                return false;
            if (!m_prefix.empty() && !string_starts_with(input, m_prefix))
                return false;
            if (!m_infix.empty() && !string_contains(input, m_infix))
                return false;
            if (!m_suffix.empty() && !string_ends_with(input, m_suffix))
                return false;
        }
        else {
            if (!m_exact.empty() && !string_equals_ignore_case(input, m_exact))
                return false;
            if (!m_prefix.empty() && !string_starts_with_ignore_case(input, m_prefix))
                return false;
            if (!m_infix.empty() && !string_contains_ignore_case(input, m_infix))
                return false;
            if (!m_suffix.empty() && !string_ends_with_ignore_case(input, m_suffix))
                return false;
        }
        return true;
    }
};

inline std::string_view get_filename_without_ext(std::string_view filename)
{
    auto dot_pos = filename.rfind('.');
    if (dot_pos == std::string_view::npos) {
        return filename;
    }
    return filename.substr(0, dot_pos);
}

inline std::string_view get_ext_from_filename(std::string_view filename)
{
    auto dot_pos = filename.rfind('.');
    if (dot_pos == std::string_view::npos) {
        return "";
    }
    return filename.substr(dot_pos + 1);
}

