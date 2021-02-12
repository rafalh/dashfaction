#pragma once

#include <cmath>
#include <patch_common/MemUtils.h>

namespace rf
{
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

        float dot_prod(const Vector3& other) const
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

    static auto& zero_vector = addr_as_ref<Vector3>(0x0173C378);

    struct Vector2
    {
        float x = 0.0f;
        float y = 0.0f;
    };
}
