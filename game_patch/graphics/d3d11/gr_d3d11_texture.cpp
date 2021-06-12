#include "gr_d3d11.h"
#include "../../bmpman/bmpman.h"

using namespace rf;

D3D11TextureManager::D3D11TextureManager(ComPtr<ID3D11Device> device) :
    device_{std::move(device)}
{
}

ComPtr<ID3D11ShaderResourceView> D3D11TextureManager::create_texture(bm::Format fmt, int w, int h, ubyte* bits, ubyte* pal)
{
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = get_dxgi_format(fmt);
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; //D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subres_data;
    ZeroMemory(&subres_data, sizeof(subres_data));

    std::unique_ptr<ubyte[]> converted_bits;
    if (desc.Format == DXGI_FORMAT_UNKNOWN) {
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        auto converted_fmt = bm::FORMAT_8888_ARGB;
        converted_bits = std::make_unique<ubyte[]>(w * h * 4);
        ::bm_convert_format(converted_bits.get(), converted_fmt, bits, fmt, w, h,
            w * bm_bytes_per_pixel(converted_fmt), w * bm_bytes_per_pixel(fmt), pal);
        subres_data.pSysMem = converted_bits.get();
        subres_data.SysMemPitch = w * bm_bytes_per_pixel(converted_fmt);
    }
    else {
        subres_data.pSysMem = bits;
        subres_data.SysMemPitch = w * bm_bytes_per_pixel(fmt);
    }

    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr = device_->CreateTexture2D(&desc, &subres_data, &texture);
    check_hr(hr, "CreateTexture2D");

    ComPtr<ID3D11ShaderResourceView> texture_view;
    hr = device_->CreateShaderResourceView(texture, nullptr, &texture_view);
    check_hr(hr, "CreateShaderResourceView");

    return texture_view;
}

ComPtr<ID3D11ShaderResourceView> D3D11TextureManager::create_texture(int bm_handle)
{
    xlog::info("Creating texture %s format %d", bm::get_filename(bm_handle), bm::get_format(bm_handle));

    int w, h;
    bm::get_dimensions(bm_handle, &w, &h);
    ubyte* bm_bits;
    ubyte* bm_pal;

    auto fmt = bm::lock(bm_handle, &bm_bits, &bm_pal);
    if (fmt == bm::FORMAT_NONE || bm_bits == nullptr || bm_pal != nullptr) {
        xlog::warn("unsupported bitmap");
        return {};
    }

    auto texture_view = create_texture(fmt, w, h, bm_bits, bm_pal);

    bm::unlock(bm_handle);

    return texture_view;
}

ID3D11ShaderResourceView* D3D11TextureManager::lookup_texture(int bm_handle)
{
    if (bm_handle < 0) {
        return nullptr;
    }
    const auto& it = texture_cache_.find(bm_handle);
    if (it != texture_cache_.end()) {
        return it->second;
    }

    ComPtr<ID3D11ShaderResourceView> texture_view = create_texture(bm_handle);
    texture_cache_.insert({bm_handle, texture_view});
    return texture_view;
}

DXGI_FORMAT D3D11TextureManager::get_dxgi_format(bm::Format fmt)
{
    switch (fmt) {
        case bm::FORMAT_565_RGB: return DXGI_FORMAT_B5G6R5_UNORM;
        //case BM_FORMAT_1555_ARGB: return DXGI_FORMAT_B5G5R5A1_UNORM; // does not work in Win7 in VirtualBox
        //case BM_FORMAT_4444_ARGB: return DXGI_FORMAT_B4G4R4A4_UNORM; // does not work in Win7 in VirtualBox
        case bm::FORMAT_8888_ARGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case bm::FORMAT_DXT1: return DXGI_FORMAT_BC1_UNORM;
        case bm::FORMAT_DXT3: return DXGI_FORMAT_BC2_UNORM;
        case bm::FORMAT_DXT5: return DXGI_FORMAT_BC3_UNORM;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

int D3D11TextureManager::lock(int bm_handle, int section, gr::LockInfo *lock, int mode)
{
    if (section != 0 || mode == gr::LOCK_READ_ONLY) {
        return 0;
    }

    int w, h;
    bm::get_dimensions(bm_handle, &w, &h);
    lock->bm_handle = bm_handle;
    lock->format = bm::get_format(bm_handle);
    lock->w = w;
    lock->h = h;
    lock->stride_in_bytes = w * bm_bytes_per_pixel(lock->format);
    lock->section = section;
    lock->data = new ubyte[w * h * bm_bytes_per_pixel(lock->format)];
    //xlog::info("locked bm %d format %d size %dx%d data %p", lock->bm_handle, lock->format, lock->w, lock->h, lock->data);
    return 1;
}

void D3D11TextureManager::unlock(gr::LockInfo *lock)
{
    //xlog::info("unlocking bm %d format %d size %dx%d data %p", lock->bm_handle, lock->format, lock->w, lock->h, lock->data);
    auto texture_view = create_texture(lock->format, lock->w, lock->h, lock->data, nullptr);
    delete[] lock->data;
    lock->data = nullptr;
    texture_cache_.insert({lock->bm_handle, texture_view});
}
