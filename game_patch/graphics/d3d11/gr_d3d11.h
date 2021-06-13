#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/os/os.h"

constexpr float d3d11_zm = 1 / 1700.0f;
// constexpr ubyte GR_DIRECT3D11 = 0x7A;

struct GpuVertex
{
    float x;
    float y;
    float z;
    // float w;
    float u0;
    float v0;
    float u1;
    float v1;
    int diffuse;
};

struct PixelShaderConstantBuffer
{
    std::array<float, 2> vcolor_mul;
    std::array<float, 2> vcolor_mul_inv;
    std::array<float, 2> tex0_mul;
    std::array<float, 2> tex0_mul_inv;
    float tex0_add_rgb;
    float output_mul_rgb;
    float alpha_test;
    float pad[1];
};

class D3D11StateManager
{
public:
    D3D11StateManager(ComPtr<ID3D11Device> device);
    ID3D11RasterizerState* get_rasterizer_state();
    ID3D11SamplerState* lookup_sampler_state(rf::gr::Mode mode, int slot);
    ID3D11BlendState* lookup_blend_state(rf::gr::Mode mode);
    ID3D11DepthStencilState* lookup_depth_stencil_state(rf::gr::Mode mode);

private:
    void init_rasterizer_state();

    ComPtr<ID3D11Device> device_;
    std::unordered_map<int, ComPtr<ID3D11SamplerState>> sampler_state_cache_;
    std::unordered_map<int, ComPtr<ID3D11BlendState>> blend_state_cache_;
    std::unordered_map<int, ComPtr<ID3D11DepthStencilState>> depth_stencil_state_cache_;
    ComPtr<ID3D11RasterizerState> rasterizer_state_;
};

class D3D11TextureManager
{
public:
    D3D11TextureManager(ComPtr<ID3D11Device> device);

    ID3D11ShaderResourceView* lookup_texture(int bm_handle);
    int lock(int bm_handle, int section, rf::gr::LockInfo *lock, int mode);
    void unlock(rf::gr::LockInfo *lock);

private:
    ComPtr<ID3D11ShaderResourceView> create_texture(rf::bm::Format fmt, int w, int h, rf::ubyte* bits, rf::ubyte* pal);
    ComPtr<ID3D11ShaderResourceView> create_texture(int bm_handle);
    DXGI_FORMAT get_dxgi_format(rf::bm::Format fmt);

    ComPtr<ID3D11Device> device_;
    std::unordered_map<int, ComPtr<ID3D11ShaderResourceView>> texture_cache_;
};

class D3D11RenderContext;

class D3D11BatchManager
{
public:
    D3D11BatchManager(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, D3D11RenderContext& render_context);
    void tmapper(int nv, rf::gr::Vertex **vertices, int tmap_flags, rf::gr::Mode mode);
    void flush();
    void bind_buffers();

private:
    void create_dynamic_vb();
    void create_dynamic_ib();
    void map_dynamic_buffers(bool vb_full, bool ib_full);
    void unmap_dynamic_buffers();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    D3D11RenderContext& render_context_;
    ComPtr<ID3D11Buffer> dynamic_vb_;
    ComPtr<ID3D11Buffer> dynamic_ib_;
    GpuVertex* mapped_vb_;
    rf::ushort* mapped_ib_;
    D3D11_PRIMITIVE_TOPOLOGY primitive_topology_;
    int current_vertex_ = 0;
    int start_index_ = 0;
    int current_index_ = 0;
    std::array<int, 2> textures_ = { -1, -1 };
    rf::gr::Mode mode_{
        rf::gr::TEXTURE_SOURCE_NONE,
        rf::gr::COLOR_SOURCE_VERTEX,
        rf::gr::ALPHA_SOURCE_VERTEX,
        rf::gr::ALPHA_BLEND_NONE,
        rf::gr::ZBUFFER_TYPE_NONE,
        rf::gr::FOG_ALLOWED,
    };
};

class D3D11RenderContext
{
public:
    D3D11RenderContext(
        ComPtr<ID3D11Device> device,
        ComPtr<ID3D11DeviceContext> context,
        D3D11StateManager& state_manager,
        D3D11TextureManager& texture_manager
    );
    void set_mode(rf::gr::Mode mode);
    void set_texture(int slot, int bm_handle);
    void set_render_target(
        const ComPtr<ID3D11RenderTargetView>& back_buffer_view,
        const ComPtr<ID3D11DepthStencilView>& depth_stencil_buffer_view
    );

private:
    void init_constant_buffer();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    D3D11StateManager& state_manager_;
    D3D11TextureManager& texture_manager_;
    ComPtr<ID3D11Buffer> ps_cbuffer_;
    int white_bm_;
};

class D3D11Renderer
{
public:
    D3D11Renderer(HWND hwnd, HMODULE d3d11_lib);
    ~D3D11Renderer();
    void window_active();
    void window_inactive();

    void bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, rf::gr::Mode mode);
    void clear();
    void zbuffer_clear();
    void set_clip();
    void flip();
    int lock(int bm_handle, int section, rf::gr::LockInfo *lock, int mode);
    void unlock(rf::gr::LockInfo *lock);
    void tmapper(int nv, rf::gr::Vertex **vertices, int tmap_flags, rf::gr::Mode mode);
    HRESULT get_device_removed_reason();

private:
    void init_device(HWND hwnd, HMODULE d3d11_lib);
    void init_back_buffer();
    void init_depth_stencil_buffer();
    void init_vertex_shader();
    void init_pixel_shader();

    ComPtr<ID3D11Device> device_;
    ComPtr<IDXGISwapChain> swap_chain_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11RenderTargetView> back_buffer_view_;
    ComPtr<ID3D11DepthStencilView> depth_stencil_buffer_view_;
    ComPtr<ID3D11VertexShader> vertex_shader_;
    ComPtr<ID3D11PixelShader> pixel_shader_;
    ComPtr<ID3D11InputLayout> input_layout_;
    std::optional<D3D11StateManager> state_manager_;
    std::optional<D3D11TextureManager> texture_manager_;
    std::optional<D3D11BatchManager> batch_manager_;
    std::optional<D3D11RenderContext> render_context_;
};

static inline void check_hr(HRESULT hr, const char* fun)
{
    void gr_d3d11_error(HRESULT hr, const char* fun);
    if (FAILED(hr)) {
        gr_d3d11_error(hr, fun);
    }
}

static inline int pack_color(int r, int g, int b, int a)
{
    // assume little endian
    return (a << 24) | (b << 16) | (g << 8) | r;
}
