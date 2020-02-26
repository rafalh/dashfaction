#pragma once

#include <cmath>
#include <cstdarg>
#include <patch_common/MemUtils.h>
#include "../utils/utils.h"

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
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(Matrix3& self)>(0x004FCE70);
            fun_ptr(*this);
        }
    };
    static_assert(sizeof(Matrix3) == 0x24);

    class File
    {
        using SelfType = File;

        char m_internal_data[0x114];

    public:
        enum SeekOrigin {
            seek_set = 0,
            seek_cur = 1,
            seek_end = 2,
        };

        File()
        {
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(SelfType*)>(0x00523940);
            fun_ptr(this);
        }

        ~File()
        {
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(SelfType*)>(0x00523960);
            fun_ptr(this);
        }

        int Open(const char* filename, int flags = 1, int unk = 9999999)
        {
            auto fun_ptr = reinterpret_cast<int(__thiscall*)(SelfType*, const char*, int, int)>(0x00524190);
            return fun_ptr(this, filename, flags, unk);
        }

        void Close()
        {
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(SelfType*)>(0x005242A0);
            fun_ptr(this);
        }

        bool CheckVersion(int min_ver) const
        {
            auto fun_ptr = reinterpret_cast<bool(__thiscall*)(const SelfType*, int)>(0x00523990);
            return fun_ptr(this, min_ver);
        }

        bool HasReadFailed() const
        {
            auto fun_ptr = reinterpret_cast<bool(__thiscall*)(const SelfType*)>(0x00524530);
            return fun_ptr(this);
        }

        int Seek(int pos, SeekOrigin origin)
        {
            auto fun_ptr = reinterpret_cast<int(__thiscall*)(const SelfType*, int, SeekOrigin)>(0x00524400);
            return fun_ptr(this, pos, origin);
        }

        int Read(void *buf, int buf_len, int min_ver = 0, int unused = 0)
        {
            auto fun_ptr = reinterpret_cast<int(__thiscall*)(SelfType*, void*, int, int, int)>(0x0052CF60);
            return fun_ptr(this, buf, buf_len, min_ver, unused);
        }

        template<typename T>
        T Read(int min_ver = 0, T def_val = 0)
        {
            if (CheckVersion(min_ver)) {
                T val;
                Read(&val, sizeof(val));
                if (!HasReadFailed()) {
                    return val;
                }
            }
            return def_val;
        }
    };

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
            auto fun_ptr = reinterpret_cast<String& (__thiscall*)(String& self)>(0x004FF3B0);
            fun_ptr(*this);
        }

        String(const char* c_str)
        {
            auto fun_ptr = reinterpret_cast<String&(__thiscall*)(String & self, const char* c_str)>(0x004FF3D0);
            fun_ptr(*this, c_str);
        }

        String(const String& str)
        {
            // Use Pod cast operator to make a clone
            m_pod = str;
        }

        ~String()
        {
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(String & self)>(0x004FF470);
            fun_ptr(*this);
        }

        operator const char*() const
        {
            return CStr();
        }

        operator Pod() const
        {
            // Make a copy
            auto fun_ptr = reinterpret_cast<Pod&(__thiscall*)(Pod & self, const Pod& str)>(0x004FF410);
            Pod pod_copy;
            fun_ptr(pod_copy, m_pod);
            return pod_copy;
        }

        String& operator=(const String& other)
        {
            auto fun_ptr = reinterpret_cast<String&(__thiscall*)(String & self, const String& str)>(0x004FFA20);
            fun_ptr(*this, other);
            return *this;
        }

        const char *CStr() const
        {
            auto fun_ptr = reinterpret_cast<const char*(__thiscall*)(const String& self)>(0x004FF480);
            return fun_ptr(*this);
        }

        int Size() const
        {
            auto fun_ptr = reinterpret_cast<int(__thiscall*)(const String& self)>(0x004FF490);
            return fun_ptr(*this);
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
            auto fun_ptr = reinterpret_cast<bool(__thiscall*)(const Timer*)>(0x004FA3F0);
            return fun_ptr(this);
        }

        void Set(int value_ms)
        {
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(Timer*, int)>(0x004FA360);
            fun_ptr(this, value_ms);
        }

        bool IsSet() const
        {
            return end_time_ms >= 0;
        }

        int GetTimeLeftMs() const
        {
            auto fun_ptr = reinterpret_cast<int(__thiscall*)(const Timer*)>(0x004FA420);
            return fun_ptr(this);
        }
    };
    static_assert(sizeof(Timer) == 0x4);

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
