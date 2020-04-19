#pragma once

#include <xlog/xlog.h>
#include <windows.h>

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

#ifdef __GNUC__
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx) __attribute__ ((format (printf, fmt_idx, va_idx)))
#else
#define PRINTF_FMT_ATTRIBUTE(fmt_idx, va_idx)
#endif

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
        size_t input_len = strlen(input);
        if (m_case_sensitive) {
            if (!m_exact.empty() && strcmp(input, m_exact.c_str()) != 0)
                return false;
            if (!m_prefix.empty() &&
                (input_len < m_prefix.size() || strncmp(input, m_prefix.c_str(), m_prefix.size()) != 0))
                return false;
            if (!m_infix.empty() && (input_len < m_infix.size() || !strstr(input, m_infix.c_str())))
                return false;
            if (!m_suffix.empty() && (input_len < m_suffix.size() || strncmp(input + input_len - m_suffix.size(),
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
    va_list args;
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

template<typename T>
class SinglyLinkedList
{
    T*& m_list;

public:
    class Iterator
    {
        T* current;
        T* first;

    public:
        typedef Iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        Iterator(pointer first) :
            current(first), first(first)
        {}

        self_type operator++()
        {
            current = current->next;
            if (current == first) {
                current = nullptr;
            }
            return *this;
        }

        self_type operator++(int junk)
        {
            self_type copy = *this;
            ++copy;
            return copy;
        }

        bool operator==(const self_type& rhs) const
        {
            return current == rhs.current;
        }

        bool operator!=(const self_type& rhs) const
        {
            return !(*this == rhs);
        }

        reference operator*() const
        {
            return *current;
        }

        pointer operator->() const
        {
            return *current;
        }
    };

    SinglyLinkedList(T*& list) : m_list(list)
    {}

    Iterator begin()
    {
        return Iterator(m_list);
    }

    Iterator end()
    {
        return Iterator(nullptr);
    }
};

template<typename T>
class DoublyLinkedList
{
    T& m_list;

public:
    class Iterator
    {
        T* current;

    public:
        typedef Iterator self_type;
        typedef T value_type;
        typedef T& reference;
        typedef T* pointer;
        typedef std::forward_iterator_tag iterator_category;
        typedef int difference_type;

        Iterator(pointer current) :
            current(current)
        {}

        self_type operator++()
        {
            current = current->next;
            return *this;
        }

        self_type operator++(int junk)
        {
            self_type copy = *this;
            ++copy;
            return copy;
        }

        bool operator==(const self_type& rhs) const
        {
            return current == rhs.current;
        }

        bool operator!=(const self_type& rhs) const
        {
            return !(*this == rhs);
        }

        reference operator*() const
        {
            return *current;
        }

        pointer operator->() const
        {
            return *current;
        }
    };

    DoublyLinkedList(T& list) : m_list(list)
    {}

    Iterator begin()
    {
        return Iterator(m_list.next);
    }

    Iterator end()
    {
        return Iterator(&m_list);
    }
};
