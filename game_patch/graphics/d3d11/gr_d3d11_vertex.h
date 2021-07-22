#pragma once

#include <vector>
#include <array>
#include <cassert>
#include <d3d11.h>

namespace df::gr::d3d11
{
    enum class VertexLayout
    {
        standard,
        character,
        transformed,
    };

    using float3 = std::array<float, 3>;

    template<VertexLayout L>
    struct VertexLayoutTrait
    {
        static std::vector<D3D11_INPUT_ELEMENT_DESC> get_desc();
    };

    struct GpuVertex
    {
        float x;
        float y;
        float z;
        float3 norm;
        int diffuse;
        float u0;
        float v0;
        float u0_pan_speed;
        float v0_pan_speed;
        float u1;
        float v1;
    };

    template<>
    inline
    std::vector<D3D11_INPUT_ELEMENT_DESC>
    VertexLayoutTrait<VertexLayout::standard>::get_desc()
    {
        return {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
    }

    struct GpuCharacterVertex0
    {
        float x;
        float y;
        float z;
    };
    static_assert(sizeof(GpuCharacterVertex0) == 12);

    struct GpuCharacterVertex1
    {
        float3 norm;
        float u0;
        float v0;
        int diffuse;
        unsigned char weights[4];
        unsigned char indices[4];
    };
    static_assert(sizeof(GpuCharacterVertex1) == 32);

    template<>
    inline
    std::vector<D3D11_INPUT_ELEMENT_DESC>
    VertexLayoutTrait<VertexLayout::character>::get_desc()
    {
        return {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
    }

    struct GpuTransformedVertex
    {
        float x;
        float y;
        float z;
        float w;
        int diffuse;
        float u0;
        float v0;
    };

    template<>
    inline
    std::vector<D3D11_INPUT_ELEMENT_DESC>
    VertexLayoutTrait<VertexLayout::transformed>::get_desc()
    {
        return {
            { "POSITIONT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
    }

    inline std::vector<D3D11_INPUT_ELEMENT_DESC> get_vertex_layout_desc(VertexLayout vertex_layout)
    {
        switch (vertex_layout) {
            case VertexLayout::standard:
                return VertexLayoutTrait<VertexLayout::standard>::get_desc();
            case VertexLayout::character:
                return VertexLayoutTrait<VertexLayout::character>::get_desc();
            case VertexLayout::transformed:
                return VertexLayoutTrait<VertexLayout::transformed>::get_desc();
            default:
                assert(false);
                return {};
        }
    }
}
