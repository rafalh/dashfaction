#pragma once

#include <cstring>
#include <format>
#include <utility>

namespace rf
{
    static auto& string_alloc = addr_as_ref<char*(unsigned size)>(0x004FF300);

    class String
    {
    public:
        // GCC follows closely Itanium ABI which requires to always pass objects by reference if class has
        // a non-trivial destructor. Therefore when passing a String by value the Pod struct should be used.
        struct Pod
        {
            int max_len;
            char* buf;
        };

    private:
        Pod m_pod;

    public:
        String()
        {
            AddrCaller{0x004FF3B0}.this_call(this);
        }

        String(const char* c_str)
        {
            AddrCaller{0x004FF3D0}.this_call(this, c_str);
        }

        String(const String& str)
        {
            AddrCaller{0x004FF410}.this_call(this, &str);
        }

        String(Pod pod) : m_pod(pod)
        {}

        ~String()
        {
            AddrCaller{0x004FF470}.this_call(this);
        }

        operator const char*() const
        {
            return c_str();
        }

        operator std::string() const
        {
            return {c_str()};
        }

        operator Pod() const
        {
            // Make a copy
            auto copy = *this;
            // Copy POD from copied string
            Pod pod = copy.m_pod;
            // Clear POD in copied string so memory pointed by copied POD is not freed
            copy.m_pod.buf = nullptr;
            copy.m_pod.max_len = 0;
            return pod;
        }

        operator std::string_view() const
        {
            return {m_pod.buf};
        }

        String& operator=(const String& other)
        {
            return AddrCaller{0x004FFA20}.this_call<String&>(this, &other);
        }

        String& operator=(const char* other)
        {
            return AddrCaller{0x004FFA80}.this_call<String&>(this, other);
        }

        [[nodiscard]] bool operator==(const String& other) const
        {
            return std::strcmp(
                m_pod.buf ? m_pod.buf : "",
                other.m_pod.buf ? other.m_pod.buf : ""
            ) == 0;
        }

        [[nodiscard]] bool operator==(const char* other) const
        {
            return std::strcmp(m_pod.buf ? m_pod.buf : "", other) == 0;
        }

        [[nodiscard]] const char *c_str() const
        {
            return AddrCaller{0x004FF480}.this_call<const char*>(this);
        }

        [[nodiscard]] int size() const
        {
            return AddrCaller{0x004FF490}.this_call<int>(this);
        }

        [[nodiscard]] bool empty() const
        {
            return size() == 0;
        }

        [[nodiscard]] String substr(int begin, int end) const
        {
            Pod result;
            AddrCaller{0x004FF590}.this_call<String*>(this, &result, begin, end);
            return {result};
        }

        [[nodiscard]] static String concat(const String& first, const String& second)
        {
            Pod result;
            AddrCaller{0x004FFB50}.c_call<String*>(&result, &first, &second);
            return {result};
        }

        template<typename... Args>
        static inline String format(std::format_string<Args...> fmt, Args&&... args)
        {
            int len = std::formatted_size(fmt, std::forward<Args>(args)...);
            int buf_size = len + 1;
            Pod result;
            result.max_len = len;
            result.buf = string_alloc(buf_size);
            std::format_to_n(result.buf, len, fmt, std::forward<Args>(args)...);
            result.buf[len] = 0;
            return {result};
        }
    };
    static_assert(sizeof(String) == 8);
}

template <>
struct std::formatter<rf::String> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const rf::String& s, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", s.c_str());
    }
};

