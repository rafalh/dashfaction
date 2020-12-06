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
    using uint = unsigned int;
    using ushort = unsigned short;
    using ubyte = unsigned char;

    struct Vector3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vector3() = default;
        Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

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

        Vector3& operator*=(float scale)
        {
            x *= scale;
            y *= scale;
            z *= scale;
            return *this;
        }

        Vector3& operator/=(float scale)
        {
            *this *= 1.0f / scale;
            return *this;
        }

        Vector3 operator-() const
        {
            return Vector3{-x, -y, -z};
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

        Vector3 operator*(float scale) const
        {
            return {x * scale, y * scale, z * scale};
        }

        void zero()
        {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
        }

        float dot_prod(const Vector3& other)
        {
            return other.x * x + other.y * y + other.z * z;
        }

        float len() const
        {
            return std::sqrt(len_sq());
        }

        float len_sq() const
        {
            return x * x + y * y + z * z;
        }

        void normalize()
        {
            *this /= len();
        }
    };
    static_assert(sizeof(Vector3) == 0xC);

    struct Matrix3
    {
        Vector3 rvec;
        Vector3 uvec;
        Vector3 fvec;

        void make_identity()
        {
            AddrCaller{0x004FCE70}.this_call(this);
        }
    };
    static_assert(sizeof(Matrix3) == 0x24);

    struct Plane
    {
        Vector3 normal;
        float offset;
    };
    static_assert(sizeof(Plane) == 0x10);

    /* String */

    static auto& StringAlloc = addr_as_ref<char*(unsigned size)>(0x004FF300);
    static auto& StringFree = addr_as_ref<void(char* ptr)>(0x004FF3A0);

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
            str.m_pod.buf = StringAlloc(buf_size);
            va_start(args, format);
            vsnprintf(str.m_pod.buf, buf_size, format, args);
            va_end(args);
            return str;
        }
    };
    static_assert(sizeof(String) == 8);

    /* Utils */

    struct Timestamp
    {
        int value = -1;

        bool elapsed() const
        {
            return AddrCaller{0x004FA3F0}.this_call<bool>(this);
        }

        void set(int value_ms)
        {
            AddrCaller{0x004FA360}.this_call(this, value_ms);
        }

        bool valid() const
        {
            return value >= 0;
        }

        int time_until() const
        {
            return AddrCaller{0x004FA420}.this_call<int>(this);
        }

        void invalidate()
        {
            AddrCaller{0x004FA3E0}.this_call(this);
        }
    };
    static_assert(sizeof(Timestamp) == 0x4);

    struct TimestampRealtime
    {
        int value = -1;

        bool elapsed() const
        {
            return AddrCaller{0x004FA560}.this_call<bool>(this);
        }

        void set(int value_ms)
        {
            AddrCaller{0x004FA4D0}.this_call(this, value_ms);
        }

        int time_until() const
        {
            return AddrCaller{0x004FA590}.this_call<int>(this);
        }

        bool valid() const
        {
            return AddrCaller{0x004FA5E0}.this_call<bool>(this);
        }

        void invalidate()
        {
            AddrCaller{0x004FA550}.this_call(this);
        }
    };
    static_assert(sizeof(TimestampRealtime) == 0x4);

    struct Color
    {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;

        constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) :
            red(r), green(g), blue(b), alpha(a) {}

        void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            this->red = r;
            this->green = g;
            this->blue = b;
            this->alpha = a;
        }
    };

    template<typename T = char>
    class VArray
    {
    private:
        int num = 0;
        int capacity = 0;
        T *elements = nullptr;

    public:
        int size() const
        {
            return num;
        }

        T& operator[](int index) const
        {
            return elements[index];
        }

        T& get(int index) const
        {
            return elements[index];
        }

        void add(T element);
    };
    static_assert(sizeof(VArray<>) == 0xC);

    template<typename T, int N>
    struct FArray
    {
        int num;
        T elements[N];
    };

    template<typename T>
    struct VList
    {
        T* head;
        int num_elements;
    };

    // RF stdlib functions are not compatible with GCC

    static auto& Free = addr_as_ref<void(void *mem)>(0x00573C71);
    static auto& Malloc = addr_as_ref<void*(uint32_t cb_size)>(0x00573B37);

    // Collide

    using IxLineSegmentBoundingBoxType = bool(const Vector3& box_min, const Vector3& box_max, const Vector3& p0,
                                       const Vector3& p1, Vector3 *hit_pt);
    static auto& ix_linesegment_boundingbox = addr_as_ref<IxLineSegmentBoundingBoxType>(0x00508B70);

    // Timer
    static auto& timer_get = addr_as_ref<int(int frequency)>(0x00504AB0);

    static auto& current_fps = addr_as_ref<float>(0x005A4018);
    static auto& frametime = addr_as_ref<float>(0x005A4014);
    static auto& framerate_min = addr_as_ref<float>(0x005A4024);
}
