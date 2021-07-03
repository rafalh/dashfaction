#pragma once

#include <array>

namespace df::gr::d3d11
{
    // 4 rows, 4 cols (column-major)
    using GpuMatrix4x4 = std::array<std::array<float, 4>, 4>;
    // 4 rows, 3 cols (column-major)
    using GpuMatrix4x3 = std::array<std::array<float, 4>, 3>;

    GpuMatrix4x4 build_identity_matrix();
    GpuMatrix4x3 build_identity_matrix43();
    GpuMatrix4x3 build_world_matrix(const rf::Vector3& pos, const rf::Matrix3& orient);
    GpuMatrix4x3 build_view_matrix(const rf::Vector3& pos, const rf::Matrix3& orient);
    GpuMatrix4x4 build_proj_matrix();
}
