#include <array>
#include <windows.h>
#include "../../rf/gr/gr.h"
#define NO_D3D8
#include "../../rf/gr/gr_direct3d.h"
#include "gr_d3d11_transform.h"

using namespace rf;

namespace df::gr::d3d11
{
    GpuMatrix4x4 build_proj_matrix()
    {
        // TODO: use variable fov and far plane
    #if 0
        // Standard D3D projection matrix, 1/z, maps near plane to 0 and far plane to 1. See:
        // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/the-direct3d-transformation-pipeline
        // https://docs.microsoft.com/en-us/windows/win32/direct3d9/projection-transform
        float z_near = 0.15f;
        float z_far = 275.0f;
        float q = z_far / (z_far - z_near);
        float fov_w = 3.14592 / 180.0f * 120.0f;
        float fov_h = fov_w * 3.0f / 4.0f;
        float w = 1 / tanf(fov_w / 2.0f);
        float h = 1 / tanf(fov_h / 2.0f);
        return {{
            {w, 0.0f, 0.0f, 0.0f},
            {0.0f, h, 0.0f, 0.0f},
            {0.0f, 0.0f, q, -q * z_near},
            {0.0f, 0.0f, 1.0f, 0.0f},
        }};
    #else
        // RF projection, 1/z, maps near plane to 1 and far plane to 0
        // It has some benefits, see "reversed-Z trick" in https://developer.nvidia.com/content/depth-precision-visualized
        using rf::gr::matrix_scale;
        using rf::gr::d3d::zm;
        return {{
            {matrix_scale.x / matrix_scale.z, 0.0f, 0.0f, 0.0f},
            {0.0f, matrix_scale.y / matrix_scale.z, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, zm / matrix_scale.z},
            {0.0f, 0.0f, 1.0f, 0.0f},
        }};
    #endif
    }
}
