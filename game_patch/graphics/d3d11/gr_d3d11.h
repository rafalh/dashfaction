#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/os/os.h"
#include "gr_d3d11_transform.h"

namespace rf
{
    struct GSolid;
    struct GRoom;
    struct VifMesh;
    struct MeshRenderParams;
    struct CharacterInstance;
}

constexpr float d3d11_zm = 1 / 1700.0f;
// constexpr ubyte GR_DIRECT3D11 = 0x7A;

class D3D11StateManager;
class D3D11ShaderManager;
class D3D11TextureManager;
class D3D11BatchManager;
class D3D11RenderContext;
class D3D11SolidRenderer;
class D3D11MeshRenderer;

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
    void render_v3d_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params);
    void render_character_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params);

private:
    void init_device(HWND hwnd, HMODULE d3d11_lib);
    void init_back_buffer();
    void init_depth_stencil_buffer();

    ComPtr<ID3D11Device> device_;
    ComPtr<IDXGISwapChain> swap_chain_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11RenderTargetView> back_buffer_view_;
    ComPtr<ID3D11DepthStencilView> depth_stencil_buffer_view_;
    std::unique_ptr<D3D11StateManager> state_manager_;
    std::unique_ptr<D3D11ShaderManager> shader_manager_;
    std::unique_ptr<D3D11TextureManager> texture_manager_;
    std::unique_ptr<D3D11BatchManager> batch_manager_;
    std::unique_ptr<D3D11RenderContext> render_context_;
    std::unique_ptr<D3D11SolidRenderer> solid_renderer_;
    std::unique_ptr<D3D11MeshRenderer> mesh_renderer_;
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
