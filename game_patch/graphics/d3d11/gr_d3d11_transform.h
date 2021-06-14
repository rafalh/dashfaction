#pragma once

#include <array>

using GrMatrix3x3 = std::array<std::array<float, 3>, 3>;
using GrMatrix4x4 = std::array<std::array<float, 4>, 4>;
using GrMatrix4x3 = std::array<std::array<float, 4>, 3>;

GrMatrix4x4 gr_d3d11_build_identity_matrix();
GrMatrix3x3 gr_d3d11_build_identity_matrix3();
GrMatrix4x4 gr_d3d11_transpose_matrix(const GrMatrix4x4& in);
GrMatrix4x4 gr_d3d11_build_camera_view_matrix();
GrMatrix4x4 gr_d3d11_build_sky_room_view_matrix();
GrMatrix4x4 gr_d3d11_build_proj_matrix();
GrMatrix4x4 gr_d3d11_build_model_matrix(const rf::Vector3& pos, const rf::Matrix3& orient);
GrMatrix3x3 gr_d3d11_build_texture_matrix(float u, float v);
GrMatrix4x3 gr_d3d11_convert_to_4x3_matrix(const GrMatrix3x3& mat);
