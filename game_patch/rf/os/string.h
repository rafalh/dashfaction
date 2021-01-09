#pragma once

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

        const char *c_str() const
        {
            return AddrCaller{0x004FF480}.this_call<const char*>(this);
        }

        int size() const
        {
            return AddrCaller{0x004FF490}.this_call<int>(this);
        }

        bool empty() const
        {
            return size() == 0;
        }

        String* substr(String *str_out, int begin, int end) const
        {
            return AddrCaller{0x004FF590}.this_call<String*>(this, str_out, begin, end);
        }

        static String* concat(String *str_out, const String& first, const String& second)
        {
            return AddrCaller{0x004FFB50}.c_call<String*>(str_out, &first, &second);
        }

        PRINTF_FMT_ATTRIBUTE(1, 2)
        static inline String format(const char* format, ...)
        {
            String str;
            va_list args;
            va_start(args, format);
            int len = vsnprintf(nullptr, 0, format, args);
            va_end(args);
            int buf_size = len + 1;
            str.m_pod.max_len = len;
            str.m_pod.buf = string_alloc(buf_size);
            va_start(args, format);
            vsnprintf(str.m_pod.buf, buf_size, format, args);
            va_end(args);
            return str;
        }
    };
    static_assert(sizeof(String) == 8);
}
