#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

namespace df::gr::d3d11
{
    class TextureManager
    {
    public:
        TextureManager(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_context);

        ID3D11ShaderResourceView* lookup_texture(int bm_handle);
        ID3D11RenderTargetView* lookup_render_target(int bm_handle);
        void save_cache();
        void flush_cache(bool force);
        void add_ref(int bm_handle);
        void remove_ref(int bm_handle);
        void remove(int bm_handle);
        bool lock(int bm_handle, int section, rf::gr::LockInfo *lock);
        void unlock(rf::gr::LockInfo *lock);
        void get_texel(int bm_handle, float u, float v, rf::gr::Color *clr);
        rf::bm::Format read_back_buffer(ID3D11Texture2D* back_buffer, int x, int y, int w, int h, rf::ubyte* data);

        void page_in(int bm_handle)
        {
            lookup_texture(bm_handle);
        }

    private:
        struct Texture
        {
            Texture() {}
            Texture(int bm_handle, DXGI_FORMAT format, ComPtr<ID3D11Texture2D> cpu_texture) :
                bm_handle{bm_handle}, format{format}, cpu_texture{cpu_texture}
            {}
            Texture(int bm_handle, DXGI_FORMAT format, ComPtr<ID3D11Texture2D> gpu_texture, ComPtr<ID3D11RenderTargetView> render_target_view) :
                bm_handle{bm_handle}, format{format}, gpu_texture{gpu_texture}, render_target_view{render_target_view}
            {}

            ID3D11ShaderResourceView* get_or_create_texture_view(ID3D11Device* device, ID3D11DeviceContext* device_context)
            {
                if (!texture_view) {
                    init_gpu_texture(device, device_context);
                }
                return texture_view;
            }

            int bm_handle = -1;
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            ComPtr<ID3D11Texture2D> cpu_texture;
            ComPtr<ID3D11Texture2D> gpu_texture;
            ComPtr<ID3D11RenderTargetView> render_target_view;
            ComPtr<ID3D11ShaderResourceView> texture_view;
            short save_cache_count = 0;
            short ref_count = 0;

            void init_gpu_texture(ID3D11Device* device, ID3D11DeviceContext* device_context);
        };

        Texture create_cpu_texture(int bm_handle, rf::bm::Format fmt, int w, int h, rf::ubyte* bits, rf::ubyte* pal);
        Texture create_render_target(int bm_handle, int w, int h);
        Texture load_texture(int bm_handle);
        Texture& get_or_load_texture(int bm_handle);
        static DXGI_FORMAT get_dxgi_format(rf::bm::Format fmt);
        static rf::bm::Format get_bm_format(DXGI_FORMAT dxgi_fmt);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> device_context_;
        std::unordered_map<int, Texture> texture_cache_;
        ComPtr<ID3D11Texture2D> back_buffer_staging_texture_;
    };
}
