#pragma once

#include <cmath>
#include <cstdarg>
#include <patch_common/MemUtils.h>
#include "../utils/string-utils.h"

#ifndef __GNUC__
#define ALIGN(n) __declspec(align(n))
#else
#define ALIGN(n) __attribute__((aligned(n)))
#endif

namespace rf
{
    struct Vector3
    {
        float x, y, z;

        Vector3() = default;

        Vector3(float x, float y, float z) :
            x(x), y(y), z(z) {}

        Vector3& operator+=(const Vector3& other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3& operator+=(float scalar)
        {
            x += scalar;
            y += scalar;
            z += scalar;
            return *this;
        }

        Vector3& operator*=(float m)
        {
            x *= m;
            y *= m;
            z *= m;
            return *this;
        }

        Vector3& operator/=(float m)
        {
            *this *= 1.0f / m;
            return *this;
        }

        Vector3 operator-() const
        {
            return Vector3(-x, -y, -z);
        }

        Vector3& operator-=(const Vector3& other)
        {
            return (*this += -other);
        }

        Vector3& operator-=(float scalar)
        {
            return (*this += -scalar);
        }

        Vector3 operator+(const Vector3& other) const
        {
            Vector3 tmp = *this;
            tmp += other;
            return tmp;
        }

        Vector3 operator-(const Vector3& other) const
        {
            Vector3 tmp = *this;
            tmp -= other;
            return tmp;
        }

        Vector3 operator+(float scalar) const
        {
            Vector3 tmp = *this;
            tmp += scalar;
            return tmp;
        }

        Vector3 operator-(float scalar) const
        {
            Vector3 tmp = *this;
            tmp -= scalar;
            return tmp;
        }

        Vector3 operator*(float m) const
        {
            return Vector3(x * m, y * m, z * m);
        }

        float DotProd(const Vector3& other)
        {
            return other.x * x + other.y * y + other.z * z;
        }

        float Len() const
        {
            return std::sqrt(LenPow2());
        }

        float LenPow2() const
        {
            return x * x + y * y + z * z;
        }

        void Normalize()
        {
            *this /= Len();
        }
    };
    static_assert(sizeof(Vector3) == 0xC);

    union Matrix3
    {
        Vector3 rows[3];
        struct
        {
            Vector3 rvec;
            Vector3 uvec;
            Vector3 fvec;
        } n;

        void SetIdentity()
        {
            AddrCaller{0x004FCE70}.this_call(this);
        }
    };
    static_assert(sizeof(Matrix3) == 0x24);

    /* String */

    static auto& StringAlloc = AddrAsRef<char*(unsigned size)>(0x004FF300);
    static auto& StringFree = AddrAsRef<void(char* ptr)>(0x004FF3A0);

    class String
    {
    public:
        // GCC follows closely Itanium ABI which requires to always pass objects by reference if class has
        // non-trivial destructor. Therefore for passing String by value Pod struct should be used.
        struct Pod
        {
            int32_t buf_size;
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
            return CStr();
        }

        operator Pod() const
        {
            // Make a copy
            auto copy = *this;
            // Copy POD from copied string
            Pod pod = copy.m_pod;
            // Clear POD in copied string so memory pointed by copied POD is not freed
            copy.m_pod.buf = nullptr;
            copy.m_pod.buf_size = 0;
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

        const char *CStr() const
        {
            return AddrCaller{0x004FF480}.this_call<const char*>(this);
        }

        int Size() const
        {
            return AddrCaller{0x004FF490}.this_call<int>(this);
        }

        bool IsEmpty() const
        {
            return Size() == 0;
        }

        String* SubStr(String *str_out, int begin, int end) const
        {
            return AddrCaller{0x004FF590}.this_call<String*>(this, str_out, begin, end);
        }

        static String* Concat(String *str_out, const String& first, const String& second)
        {
            return AddrCaller{0x004FFB50}.c_call<String*>(str_out, &first, &second);
        }

        PRINTF_FMT_ATTRIBUTE(1, 2)
        static inline String Format(const char* format, ...)
        {
            String str;
            va_list args;
            va_start(args, format);
            int size = vsnprintf(nullptr, 0, format, args) + 1;
            va_end(args);
            str.m_pod.buf_size = size;
            str.m_pod.buf = StringAlloc(str.m_pod.buf_size);
            va_start(args, format);
            vsnprintf(str.m_pod.buf, size, format, args);
            va_end(args);
            return str;
        }
    };
    static_assert(sizeof(String) == 8);

    /* Utils */

    struct Timer
    {
        int end_time_ms = -1;

        bool IsFinished() const
        {
            return AddrCaller{0x004FA3F0}.this_call<bool>(this);
        }

        void Set(int value_ms)
        {
            AddrCaller{0x004FA360}.this_call(this, value_ms);
        }

        bool IsSet() const
        {
            return end_time_ms >= 0;
        }

        int GetTimeLeftMs() const
        {
            return AddrCaller{0x004FA420}.this_call<int>(this);
        }

        void Unset()
        {
            AddrCaller{0x004FA3E0}.this_call(this);
        }
    };
    static_assert(sizeof(Timer) == 0x4);

    struct TimerApp
    {
        int end_time_ms = -1;

        bool IsFinished() const
        {
            return AddrCaller{0x004FA560}.this_call<bool>(this);
        }

        void Set(int value_ms)
        {
            AddrCaller{0x004FA4D0}.this_call(this, value_ms);
        }

        int GetTimeLeftMs() const
        {
            return AddrCaller{0x004FA590}.this_call<int>(this);
        }

        bool IsSet() const
        {
            return AddrCaller{0x004FA5E0}.this_call<bool>(this);
        }

        void Unset()
        {
            AddrCaller{0x004FA550}.this_call(this);
        }
    };
    static_assert(sizeof(TimerApp) == 0x4);

    struct Color
    {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;

        void SetRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
        {
            this->red = red;
            this->green = green;
            this->blue = blue;
            this->alpha = alpha;
        }
    };

    template<typename T = char>
    class DynamicArray
    {
    private:
        int size;
        int capacity;
        T *data;

    public:
        int Size() const
        {
            return size;
        }

        T& Get(int index) const
        {
            return data[index];
        }
    };
    static_assert(sizeof(DynamicArray<>) == 0xC);

    /* RF stdlib functions are not compatible with GCC */

    static auto& Free = AddrAsRef<void(void *mem)>(0x00573C71);
    static auto& Malloc = AddrAsRef<void*(uint32_t cb_size)>(0x00573B37);
}
