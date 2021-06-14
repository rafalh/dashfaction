#include <array>
#include <windows.h>
#include "../../rf/gr/gr.h"
#define NO_D3D8
#include "../../rf/gr/gr_direct3d.h"

using namespace rf;

auto& g_sky_room_offset = addr_as_ref<rf::Vector3>(0x0087BB00);
auto& g_sky_room_center = addr_as_ref<rf::Vector3>(0x0088FB10);

std::array<std::array<float, 4>, 4> gr_d3d11_build_identity_matrix()
{
    return {{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }};
}

std::array<std::array<float, 3>, 3> gr_d3d11_build_identity_matrix3()
{
    return {{
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
    }};
}

std::array<std::array<float, 4>, 4> gr_d3d11_transpose_matrix(const std::array<std::array<float, 4>, 4>& in)
{
    return {{
        {in[0][0], in[1][0], in[2][0], in[3][0]},
        {in[0][1], in[1][1], in[2][1], in[3][1]},
        {in[0][2], in[1][2], in[2][2], in[3][2]},
        {in[0][3], in[1][3], in[2][3], in[3][3]},
    }};
}

static std::array<std::array<float, 4>, 4> gr_d3d11_build_view_matrix(const rf::Vector3& pos, const rf::Matrix3& orient)
{
    // Note: gr_view_matrix is prescaled and because of that fog is messed so instead we use gr_eye_matrix for view matrix
    // and later use matrix scale to build projection matrix
    return {{
        {orient.rvec.x, orient.uvec.x, orient.fvec.x, 0.0f},
        {orient.rvec.y, orient.uvec.y, orient.fvec.y, 0.0f},
        {orient.rvec.z, orient.uvec.z, orient.fvec.z, 0.0f},
        {
            -(pos.x * orient.rvec.x + pos.y * orient.rvec.y + pos.z * orient.rvec.z),
            -(pos.x * orient.uvec.x + pos.y * orient.uvec.y + pos.z * orient.uvec.z),
            -(pos.x * orient.fvec.x + pos.y * orient.fvec.y + pos.z * orient.fvec.z),
            1.0f,
        },
    }};
}

std::array<std::array<float, 4>, 4> gr_d3d11_build_camera_view_matrix()
{
    return gr_d3d11_build_view_matrix(gr::eye_pos, gr::eye_matrix);
}

std::array<std::array<float, 4>, 4> gr_d3d11_build_sky_room_view_matrix()
{
    return gr_d3d11_build_view_matrix(g_sky_room_center, gr::eye_matrix);
}

std::array<std::array<float, 4>, 4> gr_d3d11_build_proj_matrix()
{
    // FIXME: this projection matrix is non-standard. See:
    // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/the-direct3d-transformation-pipeline
    // https://docs.microsoft.com/en-us/windows/win32/direct3d9/projection-transform
#if 0 // basic projection based on rf code that has unnormalized _34 (not equal 0)
    // proj_mat._11 = gr::matrix_scale.x;
    // proj_mat._12 = 0.0f;
    // proj_mat._13 = 0.0f;
    // proj_mat._14 = 0.0f;
    // proj_mat._21 = 0.0f;
    // proj_mat._22 = gr::matrix_scale.y;
    // proj_mat._23 = 0.0f;
    // proj_mat._24 = 0.0f;
    // proj_mat._31 = 0.0f;
    // proj_mat._32 = 0.0f;
    // proj_mat._33 = 0.0f;
    // proj_mat._34 = gr::matrix_scale.z;
    // proj_mat._41 = 0.0f;
    // proj_mat._42 = 0.0f;
    // proj_mat._43 = gr_d3d_zm;
    // proj_mat._44 = 0.0f;
#elif 0 // standard d3d projection matrix
    float z_near = 1.0f;
    float z_far = 1000.0f;
    float q = z_far / (z_far - z_near);
    float fov_w = 3.14592 / 180.0f * 120.0f;
    float fov_h = fov_w * 3.0f / 4.0f;
    float w = 1 / tanf(fov_w / 2.0f);
    float h = 1 / tanf(fov_h / 2.0f);
    return gr_d3d11_transpose_matrix({{
        {w, 0.0f, 0.0f, 0.0f},
        {0.0f, h, 0.0f, 0.0f},
        {0.0f, 0.0f, q, 1.0f},
        {0.0f, 0.0f, -q * z_near, 0.0f},
    }});
    rf::gr::screen.depthbuffer_type = rf::gr::DEPTHBUFFER_Z;
#else // rf projection, normalized _34 element
    return {{
        {gr::matrix_scale.x / gr::matrix_scale.z, 0.0f, 0.0f, 0.0f},
        {0.0f, gr::matrix_scale.y / gr::matrix_scale.z, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, gr::d3d::zm / gr::matrix_scale.z, 0.0f},
    }};
#endif
}

std::array<std::array<float, 4>, 4> gr_d3d11_build_model_matrix(const Vector3& pos, const Matrix3& orient)
{
    return {{
        {orient.rvec.x, orient.uvec.x, orient.fvec.x, 0.0f},
        {orient.rvec.y, orient.uvec.y, orient.fvec.y, 0.0f},
        {orient.rvec.z, orient.uvec.z, orient.fvec.z, 0.0f},
        {pos.x, pos.y, pos.z, 1.0f},
    }};
}

std::array<std::array<float, 3>, 3> gr_d3d11_build_texture_matrix(float u, float v)
{
    return {{
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {u,    v,    1.0f},
    }};
}

std::array<std::array<float, 4>, 3> gr_d3d11_convert_to_4x3_matrix(const std::array<std::array<float, 3>, 3>& mat)
{
    return {{
        {mat[0][0], mat[0][1], mat[0][2], 0.0f},
        {mat[1][0], mat[1][1], mat[1][2], 0.0f},
        {mat[2][0], mat[2][1], mat[2][2], 0.0f},
    }};
}
