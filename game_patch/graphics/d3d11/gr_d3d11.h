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
    struct VifLodMesh;
    struct MeshRenderParams;
    struct CharacterInstance;
}

namespace df::gr
{
    using namespace rf::gr;
}

namespace df::gr::d3d11
{
    // constexpr ubyte GR_DIRECT3D11 = 0x7A;

    class StateManager;
    class ShaderManager;
    class TextureManager;
    class BatchManager;
    class RenderContext;
    class SolidRenderer;
    class MeshRenderer;

    class DynamicLinkLibrary
    {
    public:
        DynamicLinkLibrary(const wchar_t* filename)
        {
            handle_ = LoadLibraryW(filename);
        }

        DynamicLinkLibrary(const DynamicLinkLibrary& other) = delete;

        DynamicLinkLibrary(DynamicLinkLibrary&& other) noexcept
            : handle_(std::exchange(other.handle_, nullptr))
        {}

        ~DynamicLinkLibrary()
        {
            if (handle_) {
                FreeLibrary(handle_);
            }
        }

        DynamicLinkLibrary& operator=(const DynamicLinkLibrary& other) = delete;

        DynamicLinkLibrary& operator=(DynamicLinkLibrary&& other) noexcept
        {
            std::swap(handle_, other.handle_);
            return *this;
        }

        operator bool() const
        {
            return handle_ != nullptr;
        }

        template<typename T>
        T get_proc_address(const char* name) const
        {
            static_assert(std::is_pointer_v<T>);
            return reinterpret_cast<T>(reinterpret_cast<void(*)()>(GetProcAddress(handle_, name)));
        }

    private:
        HMODULE handle_;
    };

    class Renderer
    {
    public:
        Renderer(HWND hwnd);
        ~Renderer();
        void window_active();
        void window_inactive();
        void set_fullscreen_state(bool fullscreen);
        void bitmap(int bm_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, rf::gr::Mode mode);
        void page_in(int bm_handle);
        void clear();
        void zbuffer_clear();
        void set_clip();
        void flip();
        void texture_save_cache();
        void texture_flush_cache(bool force);
        void texture_mark_dirty(int bm_handle);
        void texture_add_ref(int bm_handle);
        void texture_remove_ref(int bm_handle);
        bool lock(int bm_handle, int section, rf::gr::LockInfo *lock);
        void unlock(rf::gr::LockInfo *lock);
        void get_texel(int bm_handle, float u, float v, rf::gr::Color *clr);
        bool set_render_target(int bm_handle);
        rf::bm::Format read_back_buffer(int x, int y, int w, int h, rf::ubyte *data);
        void tmapper(int nv, const rf::gr::Vertex **vertices, int vertex_attributes, rf::gr::Mode mode);
        void line(const rf::gr::Vertex& v0, const rf::gr::Vertex& v1, rf::gr::Mode mode);
        void line(float x1, float y1, float x2, float y2, rf::gr::Mode mode);
        void setup_3d();
        void render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms);
        void render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient);
        void render_alpha_detail_room(rf::GRoom *room, rf::GSolid *solid);
        void render_sky_room(rf::GRoom *room);
        void render_room_liquid_surface(rf::GSolid* solid, rf::GRoom* room);
        void clear_solid_cache();
        void render_v3d_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::MeshRenderParams& params);
        void render_character_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, const rf::MeshRenderParams& params);
        void clear_vif_cache(rf::VifLodMesh *lod_mesh);
        void fog_set();
        void flush();

    private:
        void init_device();
        void init_swap_chain(HWND hwnd);
        void init_back_buffer();
        void init_depth_stencil_buffer();

        HWND hwnd_;
        DynamicLinkLibrary d3d11_lib_;
        ComPtr<ID3D11Device> device_;
        ComPtr<IDXGISwapChain> swap_chain_;
        ComPtr<ID3D11DeviceContext> context_;
        ComPtr<ID3D11Texture2D> back_buffer_;
        ComPtr<ID3D11Texture2D> msaa_render_target_;
        ComPtr<ID3D11Texture2D> default_render_target_;
        ComPtr<ID3D11RenderTargetView> default_render_target_view_;
        ComPtr<ID3D11DepthStencilView> depth_stencil_view_;
        std::unique_ptr<StateManager> state_manager_;
        std::unique_ptr<ShaderManager> shader_manager_;
        std::unique_ptr<TextureManager> texture_manager_;
        std::unique_ptr<BatchManager> batch_manager_;
        std::unique_ptr<RenderContext> render_context_;
        std::unique_ptr<SolidRenderer> solid_renderer_;
        std::unique_ptr<MeshRenderer> mesh_renderer_;
    };

    void init_error(ID3D11Device* device);
    void fatal_error(HRESULT hr, const char* fun);

    static inline void check_hr(HRESULT hr, const char* fun)
    {
        if (FAILED(hr)) {
            fatal_error(hr, fun);
        }
    }

    static inline int pack_color(const rf::Color& color)
    {
        // assume little endian
        return (color.alpha << 24) | (color.blue << 16) | (color.green << 8) | color.red;
    }

    static inline std::array<int, 2> normalize_texture_handles_for_mode(rf::gr::Mode mode, const std::array<int, 2>& textures)
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
}
