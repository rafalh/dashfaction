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

        ID3D11ShaderResourceView* lookup_texture(int bm_handle)
        {
            if (bm_handle < 0) {
                return nullptr;
            }
            Texture& texture = get_or_load_texture(bm_handle, false);
            return texture.get_or_create_texture_view(device_, device_context_);
        }

        ID3D11RenderTargetView* lookup_render_target(int bm_handle)
        {
            if (bm_handle < 0) {
                return nullptr;
            }
            Texture& texture = get_or_load_texture(bm_handle, false);
            return texture.render_target_view;
        }

        void finish_render_target(int bm_handle);
        void save_cache();
        void flush_cache(bool force);
        void add_ref(int bm_handle);
        void remove_ref(int bm_handle);
        void mark_dirty(int bm_handle);
        bool lock(int bm_handle, int section, rf::gr::LockInfo *lock);
        void unlock(rf::gr::LockInfo *lock);
        void get_texel(int bm_handle, float u, float v, rf::gr::Color *clr);
        rf::bm::Format read_back_buffer(ID3D11Texture2D* back_buffer, int x, int y, int w, int h, rf::ubyte* data);
        ComPtr<ID3D11ShaderResourceView> create_solid_color_texture(float r, float g, float b, float a);

        ID3D11ShaderResourceView* get_white_texture()
        {
            return white_texture_view_;
        }

        ID3D11ShaderResourceView* get_gray_texture()
        {
            return gray_texture_view_;
        }

        ID3D11ShaderResourceView* get_black_texture()
        {
            return black_texture_view_;
        }

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
                if (!shader_resource_view) {
                    init_shader_resource_view(device, device_context);
                }
                return shader_resource_view;
            }

            ID3D11Texture2D* get_or_create_staging_texture(ID3D11Device* device, ID3D11DeviceContext* device_context, bool copy_from_gpu)
            {
                if (!cpu_texture) {
                    init_cpu_texture(device, device_context, copy_from_gpu);
                }
                return cpu_texture;
            }

            int bm_handle = -1;
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            ComPtr<ID3D11Texture2D> cpu_texture;
            ComPtr<ID3D11Texture2D> gpu_texture;
            ComPtr<ID3D11Texture2D> gpu_ms_texture;
            ComPtr<ID3D11RenderTargetView> render_target_view;
            ComPtr<ID3D11ShaderResourceView> shader_resource_view;
            short save_cache_count = 0;
            short ref_count = 0;

            void init_shader_resource_view(ID3D11Device* device, ID3D11DeviceContext* device_context);
            void init_gpu_texture(ID3D11Device* device, ID3D11DeviceContext* device_context);
            void init_cpu_texture(ID3D11Device* device, ID3D11DeviceContext* device_context, bool copy_from_gpu);
        };

        Texture& get_or_load_texture(int bm_handle, bool staging)
        {
            // Note: bm_index will change for each animation frame but bm_handle will stay the same
            int bm_index = rf::bm::get_cache_slot(bm_handle);
            auto it = texture_cache_.find(bm_index);
            if (it != texture_cache_.end()) {
                return it->second;
            }

            auto insert_result = texture_cache_.emplace(bm_index, load_texture(bm_handle, staging));
            return insert_result.first->second;
        }

        Texture create_texture(int bm_handle, rf::bm::Format fmt, int w, int h, rf::ubyte* bits, rf::ubyte* pal, int mip_levels, bool staging);
        Texture create_render_target(int bm_handle, int w, int h);
        Texture load_texture(int bm_handle, bool staging);
        std::pair<DXGI_FORMAT, rf::bm::Format> determine_supported_texture_format(rf::bm::Format fmt);
        std::pair<DXGI_FORMAT, rf::bm::Format> get_supported_texture_format(rf::bm::Format fmt);
        static rf::bm::Format get_bm_format(DXGI_FORMAT dxgi_fmt);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> device_context_;
        std::unordered_map<int, Texture> texture_cache_;
        ComPtr<ID3D11Texture2D> back_buffer_staging_texture_;
        std::unordered_map<rf::bm::Format, std::pair<DXGI_FORMAT, rf::bm::Format>> supported_texture_format_cache_;
        ComPtr<ID3D11ShaderResourceView> white_texture_view_;
        ComPtr<ID3D11ShaderResourceView> gray_texture_view_;
        ComPtr<ID3D11ShaderResourceView> black_texture_view_;

    };
}
