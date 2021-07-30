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

    GpuMatrix4x4 build_proj_matrix();
}
