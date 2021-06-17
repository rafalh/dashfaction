#pragma once

#include <array>

namespace df::gr::d3d11
{
    using GrMatrix3x3 = std::array<std::array<float, 3>, 3>;
    using GrMatrix4x4 = std::array<std::array<float, 4>, 4>;
    using GrMatrix4x3 = std::array<std::array<float, 4>, 3>;

    GrMatrix4x4 build_identity_matrix();
    GrMatrix3x3 build_identity_matrix3();
    GrMatrix4x4 transpose_matrix(const GrMatrix4x4& in);
    GrMatrix4x4 build_camera_view_matrix();
    GrMatrix4x4 build_sky_room_view_matrix();
    GrMatrix4x4 build_proj_matrix();
    GrMatrix4x4 build_model_matrix(const rf::Vector3& pos, const rf::Matrix3& orient);
    GrMatrix3x3 build_texture_matrix(float u, float v);
    GrMatrix4x3 convert_to_4x3_matrix(const GrMatrix3x3& mat);
}
