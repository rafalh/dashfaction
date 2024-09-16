#pragma once

#include <patch_common/MemUtils.h>
#include "../math/vector.h"

#ifdef NO_D3D8
using IDirect3DTexture8 = IUnknown;
#endif

namespace rf::gr::d3d
{
    struct TextureSection
    {
        IDirect3DTexture8 *d3d_texture;
        int num_vram_bytes;
        float u_scale;
        float v_scale;
        int x;
        int y;
        int width;
        int height;
    };

    struct Texture
    {
        int bm_handle;
        short num_sections;
        short preserve_counter;
        short lock_count;
        uint8_t ref_count;
        bool reset;
        TextureSection *sections;
    };

    static auto& flush_buffers = addr_as_ref<void()>(0x00559D90);
    static auto& create_texture = addr_as_ref<int(int bm_handle, Texture& tslot)>(0x0055CC00);
    static auto& get_texture = addr_as_ref<IDirect3DTexture8*(int bm_handle)>(0x0055D1E0);
    static auto& free_texture = addr_as_ref<void(Texture& tslot)>(0x0055B640);

#if defined(DIRECT3D_VERSION) && DIRECT3D_VERSION == 0x0800
    static auto& d3d = addr_as_ref<IDirect3D8*>(0x01CFCBE0);
    static auto& device = addr_as_ref<IDirect3DDevice8*>(0x01CFCBE4);
    static auto& pp = addr_as_ref<D3DPRESENT_PARAMETERS>(0x01CFCA18);
#elif defined(DIRECT3D_VERSION)
    static auto& d3d = addr_as_ref<IUnknown*>(0x01CFCBE0);
    static auto& device = addr_as_ref<IUnknown*>(0x01CFCBE4);
#endif
    static auto& adapter_idx = addr_as_ref<uint32_t>(0x01CFCC34);
#if defined(DIRECT3D_VERSION) && DIRECT3D_VERSION == 0x0800
    static auto& device_caps = addr_as_ref<D3DCAPS8>(0x01CFCAC8);
#endif
    static auto& textures = addr_as_ref<Texture*>(0x01E65338);

    static auto& buffers_locked = addr_as_ref<bool>(0x01E652ED);
    static auto& in_optimized_drawing_proc = addr_as_ref<bool>(0x01E652EE);
    static auto& num_vertices = addr_as_ref<int>(0x01E652F0);
    static auto& num_indices = addr_as_ref<int>(0x01E652F4);
    static auto& max_hw_vertex = addr_as_ref<int>(0x01818348);
    static auto& max_hw_index = addr_as_ref<int>(0x0181834C);
    static auto& min_hw_vertex = addr_as_ref<int>(0x01E652F8);
    static auto& min_hw_index = addr_as_ref<int>(0x01E652FC);
#if defined(DIRECT3D_VERSION) && DIRECT3D_VERSION == 0x0800
    static auto& primitive_type = addr_as_ref<D3DPRIMITIVETYPE>(0x01D862A8);
#endif
    static auto& zm = addr_as_ref<float>(0x005A7DD8);
    static auto& p2t = addr_as_ref<int>(0x01CFCC18);
    static auto& use_glorad_detail_map = addr_as_ref<bool>(0x01CFCBCC);
}
