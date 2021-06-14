#pragma once

#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>

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
