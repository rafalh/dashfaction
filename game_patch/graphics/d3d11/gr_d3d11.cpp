#include <cassert>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/os/os.h"
#include "../../rf/v3d.h"
#include "../../bmpman/bmpman.h"
#include "gr_d3d11.h"
#include "gr_d3d11_context.h"
#include "gr_d3d11_shader.h"
#include "gr_d3d11_texture.h"
#include "gr_d3d11_state.h"
#include "gr_d3d11_batch.h"
#include "gr_d3d11_solid.h"
#include "gr_d3d11_mesh.h"

using namespace rf;

namespace df::gr::d3d11
{
    static HMODULE d3d11_lib;
    static std::optional<Renderer> renderer;

    void Renderer::window_active()
    {
        if (rf::gr::screen.window_mode == gr::FULLSCREEN) {
            xlog::warn("gr_d3d11_window_active SetFullscreenState");
            HRESULT hr = swap_chain_->SetFullscreenState(TRUE, nullptr);
            check_hr(hr, "SetFullscreenState");
        }
    }

    void Renderer::window_inactive()
    {
        if (rf::gr::screen.window_mode == gr::FULLSCREEN) {
            xlog::warn("gr_d3d11_window_inactive SetFullscreenState");
            HRESULT hr = swap_chain_->SetFullscreenState(FALSE, nullptr);
            check_hr(hr, "SetFullscreenState");
        }
    }

    void Renderer::init_device(HWND hwnd, HMODULE d3d11_lib)
    {
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = rf::gr::screen.window_mode == gr::FULLSCREEN ? 2 : 1;
        sd.BufferDesc.Width = rf::gr::screen.max_w;
        sd.BufferDesc.Height = rf::gr::screen.max_h;
        sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        // sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        //sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Numerator = 0;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = rf::gr::screen.window_mode == rf::gr::WINDOWED;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // FIXME: consider switching to DXGI_SWAP_EFFECT_FLIP_*

        auto pD3D11CreateDeviceAndSwapChain = reinterpret_cast<PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN>(
            reinterpret_cast<void(*)()>(GetProcAddress(d3d11_lib, "D3D11CreateDeviceAndSwapChain")));
        if (!pD3D11CreateDeviceAndSwapChain) {
            xlog::error("Failed to find D3D11CreateDeviceAndSwapChain procedure");
            abort();
        }

        // D3D_FEATURE_LEVEL feature_levels[] = {
        //     D3D_FEATURE_LEVEL_9_1,
        //     D3D_FEATURE_LEVEL_9_2,
        //     D3D_FEATURE_LEVEL_9_3,
        //     D3D_FEATURE_LEVEL_10_0,
        //     D3D_FEATURE_LEVEL_10_1,
        //     D3D_FEATURE_LEVEL_11_0,
        //     D3D_FEATURE_LEVEL_11_1
        // };

        DWORD flags = 0;
    //#ifndef NDEBUG
        // Requires Windows 10 SDK
        //flags |= D3D11_CREATE_DEVICE_DEBUG;
    //#endif
        D3D_FEATURE_LEVEL feature_level_supported;
        HRESULT hr = pD3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            // feature_levels,
            // std::size(feature_levels),
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &sd,
            &swap_chain_,
            &device_,
            &feature_level_supported,
            &context_
        );
        check_hr(hr, "D3D11CreateDeviceAndSwapChain");

        init_error(device_);

        xlog::info("D3D11 feature level: 0x%x", feature_level_supported);
    }

    void Renderer::init_back_buffer()
    {
        // Get a pointer to the back buffer
        ComPtr<ID3D11Texture2D> back_buffer;
        HRESULT hr = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&back_buffer));
        check_hr(hr, "GetBuffer");

        // Create a render-target view
        hr = device_->CreateRenderTargetView(back_buffer, NULL, &back_buffer_view_);
        check_hr(hr, "CreateRenderTargetView");
    }

    void Renderer::init_depth_stencil_buffer()
    {
        D3D11_TEXTURE2D_DESC depth_stencil_desc;
        ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
        depth_stencil_desc.Width = rf::gr::screen.max_w;
        depth_stencil_desc.Height = rf::gr::screen.max_h;
        depth_stencil_desc.MipLevels = 1;
        depth_stencil_desc.ArraySize = 1;
        depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_desc.SampleDesc.Count = 1;
        depth_stencil_desc.SampleDesc.Quality = 0;
        depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depth_stencil_desc.CPUAccessFlags = 0;
        depth_stencil_desc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> depth_stencil;
        HRESULT hr = device_->CreateTexture2D(&depth_stencil_desc, nullptr, &depth_stencil);
        check_hr(hr, "CreateTexture2D");

        hr = device_->CreateDepthStencilView(depth_stencil, nullptr, &depth_stencil_buffer_view_);
        check_hr(hr, "CreateDepthStencilView");
    }

    Renderer::Renderer(HWND hwnd, HMODULE d3d11_lib)
    {
        init_device(hwnd, d3d11_lib);
        init_back_buffer();
        init_depth_stencil_buffer();

        state_manager_ = std::make_unique<StateManager>(device_);
        shader_manager_ = std::make_unique<ShaderManager>(device_);
        texture_manager_ = std::make_unique<TextureManager>(device_, context_);
        render_context_ = std::make_unique<RenderContext>(device_, context_, *state_manager_, *shader_manager_, *texture_manager_);
        batch_manager_ = std::make_unique<BatchManager>(device_, context_, *render_context_);
        solid_renderer_ = std::make_unique<SolidRenderer>(device_, context_, *render_context_);
        mesh_renderer_ = std::make_unique<MeshRenderer>(device_, *render_context_);

        render_context_->set_render_target(back_buffer_view_, depth_stencil_buffer_view_);
        render_context_->set_cull_mode(D3D11_CULL_BACK);

        //gr::screen.mode = GR_DIRECT3D11;
        gr::screen.depthbuffer_type = gr::DEPTHBUFFER_Z;
    }

    Renderer::~Renderer()
    {
        if (context_) {
            context_->ClearState();
        }
        if (gr::screen.window_mode == gr::FULLSCREEN) {
            swap_chain_->SetFullscreenState(FALSE, nullptr);
        }
    }

    void Renderer::bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, gr::Mode mode)
    {
        // xlog::info("gr_d3d11_bitmap");
        int bm_w, bm_h;
        bm::get_dimensions(bitmap_handle, &bm_w, &bm_h);
        gr::Vertex verts[4];
        const gr::Vertex* verts_ptrs[] = {
            &verts[0],
            &verts[1],
            &verts[2],
            &verts[3],
        };
        float sx_left = static_cast<float>(gr::screen.offset_x + x);
        float sx_right = static_cast<float>(gr::screen.offset_x + x + w);
        float sy_top = static_cast<float>(gr::screen.offset_y + y);
        float sy_bottom = static_cast<float>(gr::screen.offset_y + y + h);
        float u_left = static_cast<float>(sx) / bm_w * (flip_x ? -1.0f : 1.0f);
        float u_right = static_cast<float>(sx + sw) / bm_w * (flip_x ? -1.0f : 1.0f);
        float v_top = static_cast<float>(sy) / bm_h * (flip_y ? -1.0f : 1.0f);
        float v_bottom = static_cast<float>(sy + sh) / bm_h * (flip_y ? -1.0f : 1.0f);
        verts[0].sx = sx_left;
        verts[0].sy = sy_top;
        verts[0].sw = 0.0f;
        verts[0].u1 = u_left;
        verts[0].v1 = v_top;
        verts[1].sx = sx_right;
        verts[1].sy = sy_top;
        verts[1].sw = 0.0f;
        verts[1].u1 = u_right;
        verts[1].v1 = v_top;
        verts[2].sx = sx_right;
        verts[2].sy = sy_bottom;
        verts[2].sw = 0.0f;
        verts[2].u1 = u_right;
        verts[2].v1 = v_bottom;
        verts[3].sx = sx_left;
        verts[3].sy = sy_bottom;
        verts[3].sw = 0.0f;
        verts[3].u1 = u_left;
        verts[3].v1 = v_bottom;
        std::array<int, 2> tex_handles{bitmap_handle, -1};
        batch_manager_->add_vertices(std::size(verts_ptrs), verts_ptrs, 0, tex_handles, mode);
    }

    void Renderer::page_in(int bm_handle)
    {
        texture_manager_->page_in(bm_handle);
    }

    void Renderer::clear()
    {
        render_context_->clear();
    }

    void Renderer::zbuffer_clear()
    {
        render_context_->zbuffer_clear();
    }

    void Renderer::set_clip()
    {
        batch_manager_->flush();
        render_context_->set_clip();
    }

    void Renderer::flip()
    {
        //xlog::info("gr_d3d11_flip");
        batch_manager_->flush();
        HRESULT hr = swap_chain_->Present(0, 0);
        check_hr(hr, "Present");
    }

    void Renderer::texture_save_cache()
    {
        texture_manager_->save_cache();
    }

    void Renderer::texture_flush_cache(bool force)
    {
        texture_manager_->flush_cache(force);
    }

    void Renderer::texture_add_ref(int bm_handle)
    {
        texture_manager_->add_ref(bm_handle);
    }

    void Renderer::texture_remove_ref(int bm_handle)
    {
        texture_manager_->remove_ref(bm_handle);
    }

    bool Renderer::lock(int bm_handle, int section, rf::gr::LockInfo *lock, gr::LockMode mode)
    {
        return texture_manager_->lock(bm_handle, section, lock, mode);
    }

    void Renderer::unlock(rf::gr::LockInfo *lock)
    {
        texture_manager_->unlock(lock);
    }

    void Renderer::get_texel(int bm_handle, float u, float v, rf::gr::Color *clr)
    {
        texture_manager_->get_texel(bm_handle, u, v, clr);
    }

    void Renderer::tmapper(int nv, const rf::gr::Vertex **vertices, int vertex_attributes, rf::gr::Mode mode)
    {
        std::array<int, 2> tex_handles{gr::screen.current_texture_1, gr::screen.current_texture_2};
        batch_manager_->add_vertices(nv, vertices, vertex_attributes, tex_handles, mode);
    }

    bool Renderer::render_to_texture(int bm_handle)
    {
        if (bm_handle != -1) {
            ID3D11RenderTargetView* render_target_view = texture_manager_->lookup_render_target(bm_handle);
            if (!render_target_view) {
                return false;
            }
            render_context_->set_render_target(render_target_view, depth_stencil_buffer_view_);
        }
        else {
            render_context_->set_render_target(back_buffer_view_, depth_stencil_buffer_view_);
        }
        return true;
    }

    bm::Format Renderer::read_back_buffer([[maybe_unused]] int x, [[maybe_unused]] int y, int w, int h, void *data)
    {
        if (data) {
            // TODO
            std::memset(data, 0, w * h * 4);
        }
        return bm::FORMAT_8888_ARGB;
    }

    void Renderer::setup_3d()
    {
        render_context_->update_view_proj_transform_3d();
    }

    void Renderer::render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms)
    {
        batch_manager_->flush();
        solid_renderer_->render_solid(solid, rooms, num_rooms);
    }

    void Renderer::render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient)
    {
        batch_manager_->flush();
        solid_renderer_->render_movable_solid(solid, pos, orient);
    }

    void Renderer::render_alpha_detail_room(rf::GRoom *room, rf::GSolid *solid)
    {
        batch_manager_->flush();
        solid_renderer_->render_alpha_detail(room, solid);
    }

    void Renderer::render_sky_room(rf::GRoom *room)
    {
        batch_manager_->flush();
        solid_renderer_->render_sky_room(room);
    }

    void Renderer::render_room_liquid_surface(rf::GSolid* solid, rf::GRoom* room)
    {
        batch_manager_->flush();
        solid_renderer_->render_room_liquid_surface(solid, room);
    }

    void Renderer::clear_solid_cache()
    {
        solid_renderer_->clear_cache();
    }

    void Renderer::render_v3d_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params)
    {
        batch_manager_->flush();
        mesh_renderer_->render_v3d_vif(mesh, pos, orient, params);
    }

    void Renderer::render_character_vif(rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params)
    {
        batch_manager_->flush();
        mesh_renderer_->render_character_vif(mesh, pos, orient, ci, params);
    }

    void Renderer::fog_set()
    {
        render_context_->fog_set();
    }

    void Renderer::flush()
    {
        batch_manager_->flush();
    }

    HRESULT Renderer::get_device_removed_reason()
    {
        return device_ ? device_->GetDeviceRemovedReason() : E_FAIL;
    }

    HRESULT get_device_removed_reason()
    {
        return renderer ? renderer->get_device_removed_reason() : E_FAIL;
    }
}

void gr_d3d11_msg_handler(UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_ACTIVATEAPP:
        if (w_param) {
            xlog::warn("active %x %lx", w_param, l_param);
            df::gr::d3d11::renderer->window_active();
        }
        else {
            xlog::warn("inactive %x %lx", w_param, l_param);
            df::gr::d3d11::renderer->window_inactive();
        }
    }
}

void gr_d3d11_flip()
{
    df::gr::d3d11::renderer->flip();
}

void gr_d3d11_close()
{
    xlog::info("gr_d3d11_close");
    df::gr::d3d11::renderer.reset();
    FreeLibrary(df::gr::d3d11::d3d11_lib);
}

void gr_d3d11_init(HWND hwnd)
{
    xlog::info("gr_d3d11_init");
    df::gr::d3d11::d3d11_lib = LoadLibraryW(L"d3d11.dll");
    if (!df::gr::d3d11::d3d11_lib) {
        RF_DEBUG_ERROR("Failed to load d3d11.dll");
    }

    df::gr::d3d11::renderer.emplace(hwnd, df::gr::d3d11::d3d11_lib);
    os_add_msg_handler(gr_d3d11_msg_handler);
}

bm::Format gr_d3d11_read_back_buffer(int x, int y, int w, int h, void *data)
{
    return df::gr::d3d11::renderer->read_back_buffer(x, y, w, h, data);
}

void gr_d3d11_page_in(int bm_handle)
{
    df::gr::d3d11::renderer->page_in(bm_handle);
}

void gr_d3d11_clear()
{
    df::gr::d3d11::renderer->clear();
}

void gr_d3d11_bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, gr::Mode mode)
{
    df::gr::d3d11::renderer->bitmap(bitmap_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
}

void gr_d3d11_set_clip()
{
    df::gr::d3d11::renderer->set_clip();
}

void gr_d3d11_zbuffer_clear()
{
    df::gr::d3d11::renderer->zbuffer_clear();
}

void gr_d3d11_tmapper(int nv, const gr::Vertex **vertices, int vertex_attributes, gr::Mode mode)
{
    df::gr::d3d11::renderer->tmapper(nv, vertices, vertex_attributes, mode);
}

void gr_d3d11_texture_save_cache()
{
    df::gr::d3d11::renderer->texture_save_cache();
}

void gr_d3d11_texture_flush_cache(bool force)
{
    df::gr::d3d11::renderer->texture_flush_cache(force);
}

bool gr_d3d11_lock(int bm_handle, int section, gr::LockInfo *lock, gr::LockMode mode)
{
    return df::gr::d3d11::renderer->lock(bm_handle, section, lock, mode);
}

void gr_d3d11_unlock(gr::LockInfo *lock)
{
    df::gr::d3d11::renderer->unlock(lock);
}

void gr_d3d11_get_texel(int bm_handle, float u, float v, rf::gr::Color *clr)
{
    df::gr::d3d11::renderer->get_texel(bm_handle, u, v, clr);
}

void gr_d3d11_texture_add_ref(int bm_handle)
{
    df::gr::d3d11::renderer->texture_add_ref(bm_handle);
}

void gr_d3d11_texture_remove_ref(int bm_handle)
{
    df::gr::d3d11::renderer->texture_remove_ref(bm_handle);
}

void gr_d3d11_render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms)
{
    df::gr::d3d11::renderer->render_solid(solid, rooms, num_rooms);
}

void gr_d3d11_render_movable_solid(GSolid* solid, const Vector3& pos, const Matrix3& orient)
{
    df::gr::d3d11::renderer->render_movable_solid(solid, pos, orient);
}

void gr_d3d11_render_alpha_detail_room(GRoom *room, GSolid *solid)
{
    df::gr::d3d11::renderer->render_alpha_detail_room(room, solid);
}

void gr_d3d11_render_sky_room(GRoom *room)
{
    df::gr::d3d11::renderer->render_sky_room(room);
}

void gr_d3d11_render_v3d_vif([[maybe_unused]] VifLodMesh *lod_mesh, VifMesh *mesh, const Vector3& pos, const Matrix3& orient, [[maybe_unused]] int lod_index, [[maybe_unused]] const MeshRenderParams& params)
{
    df::gr::d3d11::renderer->render_v3d_vif(mesh, pos, orient, params);
}

void gr_d3d11_render_character_vif([[maybe_unused]] VifLodMesh *lod_mesh, VifMesh *mesh, const Vector3& pos, const Matrix3& orient, const CharacterInstance *ci, [[maybe_unused]] int lod_index, [[maybe_unused]] const MeshRenderParams& params)
{
    df::gr::d3d11::renderer->render_character_vif(mesh, pos, orient, ci, params);
}

// void gr_d3d11_render_lod_vif(rf::VifLodMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, rf::CharacterInstance *ci, rf::MeshRenderParams *params)
// {
// }

void gr_d3d11_fog_set()
{
    df::gr::d3d11::renderer->fog_set();
}

bool gr_d3d11_render_to_texture(int bm_handle)
{
    return df::gr::d3d11::renderer->render_to_texture(bm_handle);
}

void gr_d3d11_render_to_back_buffer()
{
    df::gr::d3d11::renderer->render_to_texture(-1);
}

void gr_d3d11_clear_solid_render_cache()
{
    if (df::gr::d3d11::renderer) {
        df::gr::d3d11::renderer->clear_solid_cache();
    }
}

void gr_d3d11_flush()
{
    if (gr::screen.mode != 0) {
        df::gr::d3d11::renderer->flush();
    }
}

static CodeInjection g_render_room_objects_render_liquid_injection{
    0x004D4106,
    [](auto& regs) {
        GRoom* room = regs.edi;
        GSolid* solid = regs.ebx;
        df::gr::d3d11::renderer->render_room_liquid_surface(solid, room);
        regs.eip = 0x004D414F;
    },
};

static CodeInjection gr_d3d_setup_3d_injection{
    0x005473E4,
    []() {
        df::gr::d3d11::renderer->setup_3d();
    },
};

void gr_d3d11_apply_patch()
{
    g_render_room_objects_render_liquid_injection.install();
    gr_d3d_setup_3d_injection.install();

    AsmWriter{0x004F0B90}.jmp(gr_d3d11_clear_solid_render_cache); // geo_cache_clear

    // AsmWriter{0x00545960}.jmp(gr_d3d11_init);

    // write_mem<ubyte>(0x0050CBE0 + 6, GR_DIRECT3D11);
    // AsmWriter{0x0050CBE9}.call(gr_d3d11_close);

    // write_mem<ubyte>(0x0050CE2A + 6, GR_DIRECT3D11);
    // AsmWriter{0x0050CE33}.jmp(gr_d3d11_flip);

    // write_mem<ubyte>(0x0050DF80 + 6, GR_DIRECT3D11);
    // AsmWriter{0x0050DF9D}.call(gr_d3d11_tmapper);

    using namespace asm_regs;
    AsmWriter{0x00520A90}.ret(); // bink_play
    AsmWriter{0x00544FC0}.jmp(gr_d3d11_flip); // gr_d3d_flip
    AsmWriter{0x00545230}.jmp(gr_d3d11_close); // gr_d3d_close
    AsmWriter{0x00545960}.jmp(gr_d3d11_init); // gr_d3d_init
    AsmWriter{0x00546730}.jmp(gr_d3d11_read_back_buffer); // gr_d3d_read_backbuffer
    AsmWriter{0x005468C0}.jmp(gr_d3d11_fog_set); // gr_d3d_fog_set
    AsmWriter{0x00546A00}.mov(al, 1).ret(); // gr_d3d_is_mode_supported
    //AsmWriter{0x00546A40}.ret(); // gr_d3d_setup_frustum
    //AsmWriter{0x00546F60}.ret(); // gr_d3d_change_frustum
    //AsmWriter{0x00547150}.ret(); // gr_d3d_setup_3d
    //AsmWriter{0x005473F0}.ret(); // gr_d3d_start_instance
    //AsmWriter{0x00547540}.ret(); // gr_d3d_stop_instance
    //AsmWriter{0x005477A0}.ret(); // gr_d3d_project_vertex
    //AsmWriter{0x005478F0}.ret(); // gr_d3d_is_normal_facing
    //AsmWriter{0x00547960}.ret(); // gr_d3d_is_normal_facing_plane
    //AsmWriter{0x005479B0}.ret(); // gr_d3d_get_apparent_distance_from_camera
    //AsmWriter{0x005479D0}.ret(); // gr_d3d_screen_coords_from_world_coords
    AsmWriter{0x00547A60}.ret(); // gr_d3d_update_gamma_ramp
    AsmWriter{0x00547AC0}.ret(); // gr_d3d_set_texture_mip_filter
    AsmWriter{0x00550820}.jmp(gr_d3d11_page_in); // gr_d3d_page_in
    AsmWriter{0x005508C0}.jmp(gr_d3d11_clear); // gr_d3d_clear
    AsmWriter{0x00550980}.jmp(gr_d3d11_zbuffer_clear); // gr_d3d_zbuffer_clear
    AsmWriter{0x00550A30}.jmp(gr_d3d11_set_clip); // gr_d3d_set_clip
    AsmWriter{0x00550AA0}.jmp(gr_d3d11_bitmap); // gr_d3d_bitmap
    AsmWriter{0x00551450}.ret(); // gr_d3d_flush_after_color_change
    AsmWriter{0x00551460}.ret(); // gr_d3d_line
    AsmWriter{0x00551900}.jmp(gr_d3d11_tmapper); // gr_d3d_tmapper
    AsmWriter{0x005536C0}.jmp(gr_d3d11_render_sky_room);
    AsmWriter{0x00553C60}.jmp(gr_d3d11_render_movable_solid); // gr_d3d_render_movable_solid - uses gr_d3d_render_face_list
    //AsmWriter{0x00553EE0}.ret(); // gr_d3d_vfx - uses gr_poly
    //AsmWriter{0x00554BF0}.ret(); // gr_d3d_vfx_facing - uses gr_d3d_3d_bitmap_angle, gr_d3d_render_volumetric_light
    //AsmWriter{0x00555080}.ret(); // gr_d3d_vfx_glow - uses gr_d3d_3d_bitmap_angle
    AsmWriter{0x00555100}.ret(); // gr_d3d_line_vertex
    //AsmWriter{0x005551E0}.ret(); // gr_d3d_line_vec - uses gr_d3d_line_vertex
    //AsmWriter{0x00555790}.ret(); // gr_d3d_3d_bitmap - uses gr_poly
    //AsmWriter{0x00555AC0}.ret(); // gr_d3d_3d_bitmap_angle - uses gr_poly
    //AsmWriter{0x00555B20}.ret(); // gr_d3d_3d_bitmap_angle_wh - uses gr_poly
    //AsmWriter{0x00555B80}.ret(); // gr_d3d_render_volumetric_light - uses gr_poly
    //AsmWriter{0x00555DC0}.ret(); // gr_d3d_laser - uses gr_tmapper
    //AsmWriter{0x005563F0}.ret(); // gr_d3d_cylinder - uses gr_line
    //AsmWriter{0x005565D0}.ret(); // gr_d3d_cone - uses gr_line
    //AsmWriter{0x005566E0}.ret(); // gr_d3d_sphere - uses gr_line
    //AsmWriter{0x00556AB0}.ret(); // gr_d3d_chain - uses gr_poly
    //AsmWriter{0x00556F50}.ret(); // gr_d3d_line_directed - uses gr_line_vertex
    //AsmWriter{0x005571F0}.ret(); // gr_d3d_line_arrow - uses gr_line_vertex
    //AsmWriter{0x00557460}.ret(); // gr_d3d_render_particle_sys_particle - uses gr_poly, gr_3d_bitmap_angle
    //AsmWriter{0x00557D40}.ret(); // gr_d3d_render_bolts - uses gr_poly, gr_line
    //AsmWriter{0x00558320}.ret(); // gr_d3d_render_geomod_debris - uses gr_poly
    //AsmWriter{0x00558450}.ret(); // gr_d3d_render_glass_shard - uses gr_poly
    AsmWriter{0x00558550}.ret(); // gr_d3d_render_face_wireframe
    //AsmWriter{0x005585F0}.ret(); // gr_d3d_render_weapon_tracer - uses gr_poly
    //AsmWriter{0x005587C0}.ret(); // gr_d3d_poly - uses gr_d3d_tmapper
    AsmWriter{0x00558920}.ret(); // gr_d3d_render_geometry_wireframe
    AsmWriter{0x00558960}.ret(); // gr_d3d_render_geometry_in_editor
    AsmWriter{0x00558C40}.ret(); // gr_d3d_render_sel_face_in_editor
    //AsmWriter{0x00558D40}.ret(); // gr_d3d_world_poly - uses gr_d3d_poly
    //AsmWriter{0x00558E30}.ret(); // gr_d3d_3d_bitmap_stretched_square - uses gr_d3d_world_poly
    //AsmWriter{0x005590F0}.ret(); // gr_d3d_rod - uses gr_d3d_world_poly
    AsmWriter{0x005596C0}.ret(); // gr_d3d_render_face_list_colored
    AsmWriter{0x0055B520}.jmp(gr_d3d11_texture_save_cache); // gr_d3d_texture_save_cache
    AsmWriter{0x0055B550}.jmp(gr_d3d11_texture_flush_cache); // gr_d3d_texture_flush_cache
    AsmWriter{0x0055CDC0}.ret(); // gr_d3d_mark_texture_dirty
    AsmWriter{0x0055CE00}.jmp(gr_d3d11_lock); // gr_d3d_lock
    AsmWriter{0x0055CF60}.jmp(gr_d3d11_unlock); // gr_d3d_unlock
    AsmWriter{0x0055CFA0}.jmp(gr_d3d11_get_texel); // gr_d3d_get_texel
    AsmWriter{0x0055D160}.jmp(gr_d3d11_texture_add_ref); // gr_d3d_texture_add_ref
    AsmWriter{0x0055D190}.jmp(gr_d3d11_texture_remove_ref); // gr_d3d_texture_remove_ref
    AsmWriter{0x0055F5E0}.jmp(gr_d3d11_render_solid); // gr_d3d_render_static_solid
    AsmWriter{0x00561650}.ret(); // gr_d3d_render_face_list
    //AsmWriter{0x0052FA40}.jmp(gr_d3d11_render_lod_vif); // gr_d3d_render_vif_mesh
    AsmWriter{0x0052DE10}.jmp(gr_d3d11_render_v3d_vif); // gr_d3d_render_v3d_vif
    AsmWriter{0x0052E9E0}.jmp(gr_d3d11_render_character_vif); // gr_d3d_render_character_vif
    AsmWriter{0x004D34D0}.jmp(gr_d3d11_render_alpha_detail_room); // room_render_alpha_detail
    AsmWriter{0x0050E150}.jmp(gr_d3d11_flush);

    // Change size of standard structures
    write_mem<int8_t>(0x00569884 + 1, sizeof(rf::VifMesh));
    write_mem<int8_t>(0x00569732 + 1, sizeof(rf::VifLodMesh));
}
