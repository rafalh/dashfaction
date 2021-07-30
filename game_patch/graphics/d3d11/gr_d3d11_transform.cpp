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
        // FIXME: this projection matrix is non-standard. See:
        // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/the-direct3d-transformation-pipeline
        // https://docs.microsoft.com/en-us/windows/win32/direct3d9/projection-transform
    #if 0 // standard d3d projection matrix
        float z_near = 1.0f;
        float z_far = 1000.0f;
        float q = z_far / (z_far - z_near);
        float fov_w = 3.14592 / 180.0f * 120.0f;
        float fov_h = fov_w * 3.0f / 4.0f;
        float w = 1 / tanf(fov_w / 2.0f);
        float h = 1 / tanf(fov_h / 2.0f);
        return {{
            {w, 0.0f, 0.0f, 0.0f},
            {0.0f, h, 0.0f, 0.0f},
            {0.0f, 0.0f, q, 1.0f},
            {0.0f, 0.0f, -q * z_near, 0.0f},
        }};
    #else // rf projection, normalized _34 element
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
