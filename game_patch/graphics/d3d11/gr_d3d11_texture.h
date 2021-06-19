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
        bool lock(int bm_handle, int section, rf::gr::LockInfo *lock, gr::LockMode mode);
        void unlock(rf::gr::LockInfo *lock);
        void get_texel(int bm_handle, float u, float v, rf::gr::Color *clr);

    private:
        struct Texture
        {
            Texture() {}
            Texture(DXGI_FORMAT format, ComPtr<ID3D11Texture2D> cpu_texture) :
                format{format}, cpu_texture{cpu_texture}
            {}

            ID3D11ShaderResourceView* get_or_create_view(ID3D11Device* device, ID3D11DeviceContext* device_context)
            {
                if (!texture_view) {
                    init_gpu_texture(device, device_context);
                }
                return texture_view;
            }

            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            ComPtr<ID3D11Texture2D> cpu_texture;
            ComPtr<ID3D11Texture2D> gpu_texture;
            ComPtr<ID3D11ShaderResourceView> texture_view;

            void init_gpu_texture(ID3D11Device* device, ID3D11DeviceContext* device_context);
        };

        Texture create_texture(rf::bm::Format fmt, int w, int h, rf::ubyte* bits, rf::ubyte* pal);
        Texture load_texture(int bm_handle);
        Texture& get_or_load_texture(int bm_handle);
        static DXGI_FORMAT get_dxgi_format(rf::bm::Format fmt);
        static rf::bm::Format get_bm_format(DXGI_FORMAT dxgi_fmt);

        ComPtr<ID3D11Device> device_;
        ComPtr<ID3D11DeviceContext> device_context_;
        std::unordered_map<int, Texture> texture_cache_;
    };
}
