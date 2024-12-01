#pragma once

#include <array>
#include "../../rf/math/vector.h"
#include "../../rf/math/matrix.h"

namespace df::gr::d3d11
{
    // 4 rows, 4 cols (column-major)
    using GpuMatrix4x4 = std::array<std::array<float, 4>, 4>;
    // 4 rows, 3 cols (column-major)
    using GpuMatrix4x3 = std::array<std::array<float, 4>, 3>;

    class Projection
    {
        float sx_ = 1.0f;
        float sy_ = 1.0f;
        float sz_ = -0.1f;
        float tz_ = 1.0f;
        float zf_ = 1.0f;

    public:
        Projection() = default;
        Projection(float sx, float sy, float zn, float zf) :
            sx_{sx}, sy_{sy}, sz_{-zn / (zf - zn)}, tz_{zf * zn / (zf - zn)}, zf_{zf}
        {}

        float project_z(float z) const
        {
            return (z * sz_ + tz_) / z;
        }

        rf::Vector3 project(rf::Vector3 p) const
        {
            return {
                p.x * sx_ / p.z,
                p.y * sy_ / p.z,
                project_z(p.z),
            };
        }

        float unproject_z(float pz) const
        {
            // (z * sz_ + tz_) / z == pz
            // pz * z = z * sz_ + tz_
            // pz * z - sz_ * z = tz_
            // z * (pz - sz_) = tz_
            // z = tz_ / (pz - sz_)
            return tz_ / (pz - sz_);
        }

        GpuMatrix4x4 matrix() const
        {
            // RF projection (1/z) uses reversed-Z trick, maps near plane to 1 and infinity to 0
            // It has some benefits, see "reversed-Z trick" in
            // https://developer.nvidia.com/content/depth-precision-visualized To support far plane clipping change it a
            // bit and map far plane to 0 instead of infinity Matrix construction:
            // https://iolite-engine.com/blog_posts/reverse_z_cheatsheet
            return {{
                {sx_, 0.0f, 0.0f, 0.0f},
                {0.0f, sy_, 0.0f, 0.0f},
                {0.0f, 0.0f, sz_, tz_},
                {0.0f, 0.0f, 1.0f, 0.0f},
            }};
        }

        float z_far() const
        {
            return zf_;
        }
    };

    inline GpuMatrix4x4 build_identity_matrix()
    {
        return {{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        }};
    }

    inline GpuMatrix4x3 build_identity_matrix43()
    {
        return {{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
        }};
    }

    inline GpuMatrix4x3 build_world_matrix(const rf::Vector3& pos, const rf::Matrix3& orient)
    {
        return {{
            {orient.rvec.x, orient.uvec.x, orient.fvec.x, pos.x},
            {orient.rvec.y, orient.uvec.y, orient.fvec.y, pos.y},
            {orient.rvec.z, orient.uvec.z, orient.fvec.z, pos.z},
        }};
    }

    inline GpuMatrix4x3 build_view_matrix(const rf::Vector3& pos, const rf::Matrix3& orient)
    {
        rf::Vector3 translation{
            -(pos.x * orient.rvec.x + pos.y * orient.rvec.y + pos.z * orient.rvec.z),
            -(pos.x * orient.uvec.x + pos.y * orient.uvec.y + pos.z * orient.uvec.z),
            -(pos.x * orient.fvec.x + pos.y * orient.fvec.y + pos.z * orient.fvec.z),
        };
        return {{
            {orient.rvec.x, orient.rvec.y, orient.rvec.z, translation.x},
            {orient.uvec.x, orient.uvec.y, orient.uvec.z, translation.y},
            {orient.fvec.x, orient.fvec.y, orient.fvec.z, translation.z},
        }};
    }
}
