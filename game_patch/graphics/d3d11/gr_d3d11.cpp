#include <cassert>
#include <dxgi1_4.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/v3d.h"
#include "../../bmpman/bmpman.h"
#include "../../main/main.h"
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
    constexpr DXGI_FORMAT back_buffer_format = DXGI_FORMAT_B8G8R8A8_UNORM;

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

    void Renderer::init_device()
    {
        auto pD3D11CreateDevice = d3d11_lib_.get_proc_address<PFN_D3D11_CREATE_DEVICE>("D3D11CreateDevice");
        if (!pD3D11CreateDevice) {
            xlog::error("Failed to find D3D11CreateDevice procedure");
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
        HRESULT hr = pD3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            // feature_levels,
            // std::size(feature_levels),
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &device_,
            &feature_level_supported,
            &context_
        );
        check_hr(hr, "D3D11CreateDeviceAndSwapChain");

        init_error(device_);

        xlog::info("D3D11 feature level: 0x%x", feature_level_supported);
    }

    void Renderer::init_swap_chain(HWND hwnd)
    {
        ComPtr<IDXGIDevice> dxgi_device;
        HRESULT hr = device_->QueryInterface(&dxgi_device);
        check_hr(hr, "QueryInterface IDXGIDevice");

        ComPtr<IDXGIAdapter> dxgi_adapter;
        hr = dxgi_device->GetAdapter(&dxgi_adapter);
        check_hr(hr, "GetAdapter");

        ComPtr<IDXGIFactory> dxgi_factory;
        hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgi_factory));
        check_hr(hr, "GetParent");

        ComPtr<IDXGIFactory2> dxgi_factory2;
        ComPtr<IDXGIFactory3> dxgi_factory3;
        ComPtr<IDXGIFactory4> dxgi_factory4;
        dxgi_factory->QueryInterface(&dxgi_factory2);
        dxgi_factory->QueryInterface(&dxgi_factory3);
        dxgi_factory->QueryInterface(&dxgi_factory4);

        if (dxgi_factory2) {
            DXGI_SWAP_CHAIN_DESC1 sc_desc1;
            ZeroMemory(&sc_desc1, sizeof(sc_desc1));
            sc_desc1.Width = rf::gr::screen.max_w;
            sc_desc1.Height = rf::gr::screen.max_h;
            sc_desc1.Format = back_buffer_format;
            // Note: flip modes do not support multi-sampling so always use one sample for backbuffer
            // When MSAA is enabled we will create a separate render target with multi-sampling enabled
            sc_desc1.SampleDesc.Count = 1;
            sc_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sc_desc1.BufferCount = 2;
            if (dxgi_factory4) {
                sc_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            }
            else if (dxgi_factory3) {
                sc_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            }
            else {
                sc_desc1.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            }
            xlog::info("D3D11 swap effect: %d", sc_desc1.SwapEffect);
            sc_desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC sc_fs_desc;
            ZeroMemory(&sc_fs_desc, sizeof(sc_fs_desc));
            sc_fs_desc.Windowed = rf::gr::screen.window_mode == rf::gr::WINDOWED;

            ComPtr<IDXGISwapChain1> swap_chain1;
            hr = dxgi_factory2->CreateSwapChainForHwnd(device_, hwnd, &sc_desc1, &sc_fs_desc, nullptr, &swap_chain1);
            check_hr(hr, "CreateSwapChainForHwnd");
            swap_chain1->QueryInterface(&swap_chain_);
        }
        else {
            DXGI_SWAP_CHAIN_DESC sd;
            ZeroMemory(&sd, sizeof(sd));
            sd.BufferCount = rf::gr::screen.window_mode == gr::FULLSCREEN ? 2 : 1;
            sd.BufferDesc.Width = rf::gr::screen.max_w;
            sd.BufferDesc.Height = rf::gr::screen.max_h;
            sd.BufferDesc.Format = back_buffer_format;
            sd.BufferDesc.RefreshRate.Numerator = 0;
            sd.BufferDesc.RefreshRate.Denominator = 1;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = hwnd;
            sd.SampleDesc.Count = 1;
            sd.SampleDesc.Quality = 0;
            sd.Windowed = rf::gr::screen.window_mode == rf::gr::WINDOWED;
            sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            hr = dxgi_factory->CreateSwapChain(device_, &sd, &swap_chain_);
            check_hr(hr, "CreateSwapChain");
        }
    }

    void Renderer::init_back_buffer()
    {
        // Get a pointer to the back buffer
        HRESULT hr = swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&back_buffer_));
        check_hr(hr, "GetBuffer");

        // Create a render-target view
        if (g_game_config.msaa) {
            D3D11_TEXTURE2D_DESC desc;
            back_buffer_->GetDesc(&desc);
            desc.SampleDesc.Count = g_game_config.msaa;
            hr = device_->CreateTexture2D(&desc, nullptr, &msaa_render_target_);
            check_hr(hr, "CreateTexture2D msaa render target");
            default_render_target_ = msaa_render_target_;

            CD3D11_RENDER_TARGET_VIEW_DESC view_desc{D3D11_RTV_DIMENSION_TEXTURE2DMS};
            hr = device_->CreateRenderTargetView(default_render_target_, &view_desc, &default_render_target_view_);
            check_hr(hr, "CreateRenderTargetView msaa render target");
        }
        else {
            default_render_target_ = back_buffer_;
            hr = device_->CreateRenderTargetView(default_render_target_, nullptr, &default_render_target_view_);
            check_hr(hr, "CreateRenderTargetView back buffer");
        }
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
        depth_stencil_desc.SampleDesc.Count = std::max(g_game_config.msaa.value(), 1u);
        depth_stencil_desc.SampleDesc.Quality = 0;
        depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depth_stencil_desc.CPUAccessFlags = 0;
        depth_stencil_desc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> depth_stencil;
        HRESULT hr = device_->CreateTexture2D(&depth_stencil_desc, nullptr, &depth_stencil);
        check_hr(hr, "CreateTexture2D depth stencil");

        D3D11_DEPTH_STENCIL_VIEW_DESC view_desc;
        ZeroMemory(&view_desc, sizeof(view_desc));
        view_desc.ViewDimension = g_game_config.msaa ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
        hr = device_->CreateDepthStencilView(depth_stencil, &view_desc, &depth_stencil_view_);
        check_hr(hr, "CreateDepthStencilView");
    }

    Renderer::Renderer(HWND hwnd) : d3d11_lib_{L"d3d11.dll"}
    {
        if (!d3d11_lib_) {
            RF_DEBUG_ERROR("Failed to load d3d11.dll");
        }
        init_device();
        init_swap_chain(hwnd);
        init_back_buffer();
        init_depth_stencil_buffer();

        state_manager_ = std::make_unique<StateManager>(device_);
        shader_manager_ = std::make_unique<ShaderManager>(device_);
        texture_manager_ = std::make_unique<TextureManager>(device_, context_);
        render_context_ = std::make_unique<RenderContext>(device_, context_, *state_manager_, *shader_manager_, *texture_manager_);
        batch_manager_ = std::make_unique<BatchManager>(device_, *shader_manager_, *render_context_);
        solid_renderer_ = std::make_unique<SolidRenderer>(device_, *shader_manager_, *render_context_);
        mesh_renderer_ = std::make_unique<MeshRenderer>(device_, *shader_manager_, *render_context_);

        render_context_->set_render_target(default_render_target_view_, depth_stencil_view_);
        render_context_->set_cull_mode(D3D11_CULL_BACK);

        //gr::screen.mode = GR_DIRECT3D11;
        gr::screen.depthbuffer_type = gr::DEPTHBUFFER_Z;

        // Disable software vertex clipping
        auto& gr_needs_software_clipping = addr_as_ref<bool>(0x005A445A);
        gr_needs_software_clipping = false;
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

    void Renderer::bitmap(int bm_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, gr::Mode mode)
    {
        // xlog::info("gr_d3d11_bitmap");
        int bm_w, bm_h;
        bm::get_dimensions(bm_handle, &bm_w, &bm_h);
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
        float u_left = static_cast<float>(sx + (flip_x ? sw : 0)) / bm_w;
        float u_right = static_cast<float>(sx + (flip_x ? 0 : sw)) / bm_w;
        float v_top = static_cast<float>(sy + (flip_y ? sh : 0)) / bm_h;
        float v_bottom = static_cast<float>(sy + (flip_y ? 0 : sh)) / bm_h;
        verts[0].sx = sx_left;
        verts[0].sy = sy_top;
        verts[0].sw = 1.0f;
        verts[0].u1 = u_left;
        verts[0].v1 = v_top;
        verts[1].sx = sx_right;
        verts[1].sy = sy_top;
        verts[1].sw = 1.0f;
        verts[1].u1 = u_right;
        verts[1].v1 = v_top;
        verts[2].sx = sx_right;
        verts[2].sy = sy_bottom;
        verts[2].sw = 1.0f;
        verts[2].u1 = u_right;
        verts[2].v1 = v_bottom;
        verts[3].sx = sx_left;
        verts[3].sy = sy_bottom;
        verts[3].sw = 1.0f;
        verts[3].u1 = u_left;
        verts[3].v1 = v_bottom;
        std::array<int, 2> tex_handles{bm_handle, -1};
        batch_manager_->add_poly(std::size(verts_ptrs), verts_ptrs, 0, tex_handles, mode);
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
        if (msaa_render_target_) {
            context_->ResolveSubresource(back_buffer_, 0, msaa_render_target_, 0, back_buffer_format);
        }
        UINT sync_interval = g_game_config.vsync ? 1 : 0;
        HRESULT hr = swap_chain_->Present(sync_interval, 0);
        check_hr(hr, "Present");
        // Flip swap effect clears render target after Present call
        render_context_->set_render_target(default_render_target_view_, depth_stencil_view_);
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

    void Renderer::texture_remove(int bm_handle)
    {
        texture_manager_->remove(bm_handle);
    }

    bool Renderer::lock(int bm_handle, int section, rf::gr::LockInfo *lock)
    {
        return texture_manager_->lock(bm_handle, section, lock);
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
        batch_manager_->add_poly(nv, vertices, vertex_attributes, tex_handles, mode);
    }

    void Renderer::line(const rf::gr::Vertex& v0, const rf::gr::Vertex& v1, rf::gr::Mode mode)
    {
        const rf::gr::Vertex* vertices[] = {&v0, &v1};
        batch_manager_->add_line(vertices, mode);
    }

    void Renderer::line(float x1, float y1, float x2, float y2, rf::gr::Mode mode)
    {
        rf::gr::Vertex verts[2];
        verts[0].sx = x1;
        verts[0].sy = y1;
        verts[0].sw = 1.0f;
        verts[1].sx = x2;
        verts[1].sy = y2;
        verts[1].sw = 1.0f;
        line(verts[0], verts[1], mode);
    }

    bool Renderer::set_render_target(int bm_handle)
    {
        batch_manager_->flush();
        if (bm_handle != -1) {
            ID3D11RenderTargetView* render_target_view = texture_manager_->lookup_render_target(bm_handle);
            if (!render_target_view) {
                return false;
            }
            render_context_->set_render_target(render_target_view, depth_stencil_view_);
        }
        else {
            render_context_->set_render_target(default_render_target_view_, depth_stencil_view_);
        }
        return true;
    }

    bm::Format Renderer::read_back_buffer([[maybe_unused]] int x, [[maybe_unused]] int y, int w, int h, rf::ubyte *data)
    {
        batch_manager_->flush();
        return texture_manager_->read_back_buffer(back_buffer_, x, y, w, h, data);
    }

    void Renderer::setup_3d()
    {
        render_context_->update_view_proj_transform();
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

    void Renderer::render_v3d_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params)
    {
        batch_manager_->flush();
        mesh_renderer_->render_v3d_vif(lod_mesh, mesh, pos, orient, params);
    }

    void Renderer::render_character_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params)
    {
        batch_manager_->flush();
        mesh_renderer_->render_character_vif(lod_mesh, mesh, pos, orient, ci, params);
    }

    void Renderer::clear_vif_cache(rf::VifLodMesh *lod_mesh)
    {
        mesh_renderer_->clear_vif_cache(lod_mesh);
    }

    void Renderer::fog_set()
    {
        render_context_->fog_set();
    }

    void Renderer::flush()
    {
        batch_manager_->flush();
    }
}
