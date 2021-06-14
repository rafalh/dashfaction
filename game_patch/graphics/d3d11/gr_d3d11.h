#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/os/os.h"

constexpr float d3d11_zm = 1 / 1700.0f;
// constexpr ubyte GR_DIRECT3D11 = 0x7A;

namespace rf
{
    struct GRoom;
    struct GSolid;
    struct GDecal;
}

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

struct VertexShaderUniforms
{
    std::array<std::array<float, 4>, 4> model_mat;
    std::array<std::array<float, 4>, 4> view_mat;
    std::array<std::array<float, 4>, 4> proj_mat;
};

struct TexCoordTransformUniform
{
    std::array<std::array<float, 4>, 3> mat;
};

struct PixelShaderUniforms
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

using GrMatrix3 = std::array<std::array<float, 3>, 3>;
using GrMatrix4 = std::array<std::array<float, 4>, 4>;

class D3D11RenderContext
{
public:
    using TextureTransform = std::array<std::array<float, 3>, 3>;

    D3D11RenderContext(
        ComPtr<ID3D11Device> device,
        ComPtr<ID3D11DeviceContext> context,
        D3D11StateManager& state_manager,
        D3D11TextureManager& texture_manager
    );
    void set_mode_and_textures(rf::gr::Mode mode, int bm_handle1, int bm_handle2);
    void set_mode(rf::gr::Mode mode);
    void set_texture(int slot, int bm_handle);
    void set_render_target(
        const ComPtr<ID3D11RenderTargetView>& back_buffer_view,
        const ComPtr<ID3D11DepthStencilView>& depth_stencil_buffer_view
    );
    void update_vs_uniforms(const VertexShaderUniforms& uniforms);
    void set_vertex_buffer(ID3D11Buffer* vb);
    void set_index_buffer(ID3D11Buffer* ib);
    void set_texture_transform(const GrMatrix3& transform);

    ID3D11DeviceContext* device_context()
    {
        return context_;
    }

private:
    void init_ps_cbuffer();
    void init_vs_cbuffer();

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    D3D11StateManager& state_manager_;
    D3D11TextureManager& texture_manager_;
    ComPtr<ID3D11Buffer> vs_cbuffer_;
    ComPtr<ID3D11Buffer> ps_cbuffer_;
    ComPtr<ID3D11Buffer> texture_transform_cbuffer_;
    int white_bm_;
    GrMatrix3 current_texture_transform_;
};

class GRenderCacheBuilder;
enum class FaceRenderType { opaque, alpha, liquid };

class GRenderCache
{
public:
    GRenderCache(const GRenderCacheBuilder& builder, rf::gr::Mode mode, ID3D11Device* device);
    void render(FaceRenderType what, D3D11RenderContext& context);

private:
    // struct GpuVertex
    // {
    //     float x;
    //     float y;
    //     float z;
    //     int diffuse;
    //     float u0;
    //     float v0;
    //     float u1;
    //     float v1;
    // };

    struct Batch
    {
        int min_index;
        int start_index;
        int num_verts;
        int num_tris;
        int texture_1;
        int texture_2;
        float u_pan_speed;
        float v_pan_speed;
        rf::gr::Mode mode;
    };

    std::vector<Batch> opaque_batches_;
    std::vector<Batch> alpha_batches_;
    std::vector<Batch> liquid_batches_;
    ComPtr<ID3D11Buffer> vb_;
    ComPtr<ID3D11Buffer> ib_;
};

class RoomRenderCache
{
public:
    RoomRenderCache(rf::GSolid* solid, rf::GRoom* room, ID3D11Device* device);
    void render(FaceRenderType render_type, ID3D11Device* device, D3D11RenderContext& context);
    rf::GRoom* room() const;

private:
    char padding_[0x20];
    int state_ = 0; // modified by the game engine during geomod operation
    rf::GRoom* room_;
    rf::GSolid* solid_;
    std::optional<GRenderCache> cache_;

    void update(ID3D11Device* device);
    bool invalid() const;
};

class D3D11SolidRenderer
{
public:
    D3D11SolidRenderer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context, D3D11RenderContext& render_context);
    void render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms);
    void render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
    void render_sky_room(rf::GRoom *room);
    void render_alpha_detail(rf::GRoom *room, rf::GSolid *solid);
    void render_room_liquid_surface(rf::GSolid* solid, rf::GRoom* room);
    void clear_cache();

private:
    void before_render(const rf::Vector3& pos, const rf::Matrix3& orient, bool is_skyroom);
    void after_render();
    void render_room_faces(rf::GSolid* solid, rf::GRoom* room, FaceRenderType render_type);
    void render_detail(rf::GSolid* solid, rf::GRoom* room, bool alpha);
    void render_dynamic_decals(rf::GRoom** rooms, int num_rooms);
    void render_dynamic_decal(rf::GDecal* decal, rf::GRoom* room);

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    D3D11RenderContext& render_context_;
    std::vector<std::unique_ptr<RoomRenderCache>> room_cache_;
    std::vector<std::unique_ptr<GRenderCache>> mover_render_cache_;
    std::vector<std::unique_ptr<GRenderCache>> detail_render_cache_;
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
    void render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms);
    void render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
    void render_alpha_detail_room(rf::GRoom *room, rf::GSolid *solid);
    void render_sky_room(rf::GRoom *room);
    void render_room_liquid_surface(rf::GSolid* solid, rf::GRoom* room);
    void clear_solid_cache();

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
    std::optional<D3D11SolidRenderer> solid_renderer_;
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

static inline std::array<int, 2> gr_d3d11_normalize_texture_handles_for_mode(rf::gr::Mode mode, const std::array<int, 2>& textures)
{
    switch (mode.get_texture_source()) {
        case rf::gr::TEXTURE_SOURCE_NONE:
            return {-1, -1};
        case rf::gr::TEXTURE_SOURCE_WRAP:
        case rf::gr::TEXTURE_SOURCE_CLAMP:
        case rf::gr::TEXTURE_SOURCE_CLAMP_NO_FILTERING:
            return {textures[0], -1};
        case rf::gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0:
        case rf::gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X:
        case rf::gr::TEXTURE_SOURCE_CLAMP_1_CLAMP_0:
        case rf::gr::TEXTURE_SOURCE_MT_U_WRAP_V_CLAMP:
        case rf::gr::TEXTURE_SOURCE_MT_U_CLAMP_V_WRAP:
        case rf::gr::TEXTURE_SOURCE_MT_WRAP_TRILIN:
        case rf::gr::TEXTURE_SOURCE_MT_CLAMP_TRILIN:
            return textures;
        default:
            return {-1, -1};
    }
}

std::array<std::array<float, 4>, 4> gr_d3d11_build_identity_matrix();
std::array<std::array<float, 3>, 3> gr_d3d11_build_identity_matrix3();
std::array<std::array<float, 4>, 4> gr_d3d11_transpose_matrix(const std::array<std::array<float, 4>, 4>& in);
std::array<std::array<float, 4>, 4> gr_d3d11_build_camera_view_matrix();
std::array<std::array<float, 4>, 4> gr_d3d11_build_sky_room_view_matrix();
std::array<std::array<float, 4>, 4> gr_d3d11_build_proj_matrix();
std::array<std::array<float, 4>, 4> gr_d3d11_build_model_matrix(const rf::Vector3& pos, const rf::Matrix3& orient);
std::array<std::array<float, 3>, 3> gr_d3d11_build_texture_matrix(float u, float v);
std::array<std::array<float, 4>, 3> gr_d3d11_convert_to_4x3_matrix(const std::array<std::array<float, 3>, 3>& mat);
