#include <cstring>
#include "gr_d3d11.h"
#include "gr_d3d11_texture.h"
#include "../../bmpman/bmpman.h"

using namespace rf;

namespace df::gr::d3d11
{
    TextureManager::TextureManager(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_context) :
        device_{std::move(device)}, device_context_{std::move(device_context)}
    {
        texture_cache_.reserve(512);
    }

    TextureManager::Texture TextureManager::create_cpu_texture(int bm_handle, bm::Format fmt, int w, int h, ubyte* bits, ubyte* pal)
    {
        CD3D11_TEXTURE2D_DESC desc{
            get_dxgi_format(fmt),
            static_cast<UINT>(w),
            static_cast<UINT>(h),
            1, // arraySize
            1, // mipLevels
            0, // bindFlags
            D3D11_USAGE_STAGING,
            D3D11_CPU_ACCESS_READ|D3D11_CPU_ACCESS_WRITE,
        };
        // TODO: mapmaps?

        bm::Format gpu_fmt = fmt;
        if (desc.Format == DXGI_FORMAT_UNKNOWN) {
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            gpu_fmt = bm::FORMAT_8888_ARGB;
        }

        D3D11_SUBRESOURCE_DATA subres_data{nullptr, 0, 0};
        std::unique_ptr<ubyte[]> converted_bits;
        D3D11_SUBRESOURCE_DATA* subres_data_ptr = nullptr;
        if (bits) {
            if (gpu_fmt != fmt) {
                xlog::trace("Converting texture %d -> %d", fmt, gpu_fmt);
                converted_bits = std::make_unique<ubyte[]>(w * h * 4);
                ::bm_convert_format(converted_bits.get(), gpu_fmt, bits, fmt, w, h,
                    w * bm_bytes_per_pixel(gpu_fmt), w * bm_bytes_per_pixel(fmt), pal);
                subres_data.pSysMem = converted_bits.get();
                subres_data.SysMemPitch = w * bm_bytes_per_pixel(gpu_fmt);
            }
            else {
                xlog::trace("Creating texture without conversion: format %d", fmt);
                subres_data.pSysMem = bits;
                subres_data.SysMemPitch = w * bm_bytes_per_pixel(fmt);
            }
            subres_data_ptr = &subres_data;
        }
        else {
            xlog::trace("Creating uninitialized texture");
        }

        ComPtr<ID3D11Texture2D> cpu_texture;
        HRESULT hr = device_->CreateTexture2D(&desc, subres_data_ptr, &cpu_texture);
        check_hr(hr, "CreateTexture2D cpu");
        return {bm_handle, desc.Format, cpu_texture};
    }

    TextureManager::Texture TextureManager::create_render_target(int bm_handle, int w, int h)
    {
        CD3D11_TEXTURE2D_DESC desc{
            DXGI_FORMAT_R8G8B8A8_UNORM,
            static_cast<UINT>(w),
            static_cast<UINT>(h),
            1, // arraySize
            1, // mipLevels
            D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE,
        };
        ComPtr<ID3D11Texture2D> gpu_texture;
        HRESULT hr = device_->CreateTexture2D(&desc, nullptr, &gpu_texture);
        check_hr(hr, "CreateTexture2D render target");

        ComPtr<ID3D11RenderTargetView> render_target_view;
        hr = device_->CreateRenderTargetView(gpu_texture, nullptr, &render_target_view);
        check_hr(hr, "CreateRenderTargetView");

        return {bm_handle, desc.Format, std::move(gpu_texture), std::move(render_target_view)};
    }

    TextureManager::Texture TextureManager::load_texture(int bm_handle)
    {
        xlog::info("Creating texture for bitmap %s handle %d format %d", bm::get_filename(bm_handle), bm_handle, bm::get_format(bm_handle));

        int w = 0, h = 0;
        bm::get_dimensions(bm_handle, &w, &h);
        if (w <= 0 || h <= 0) {
            xlog::warn("Bad bitmap dimensions");
            return {};
        }

        if (bm::get_format(bm_handle) == bm::FORMAT_RENDER_TARGET) {
            xlog::info("Creating render target");
            return create_render_target(bm_handle, w, h);
        }

        if (bm::get_type(bm_handle) == bm::TYPE_USER) {
            xlog::trace("Creating user bitmap texture");
            auto fmt = bm::get_format(bm_handle);
            auto texture = create_cpu_texture(bm_handle, fmt, w, h, nullptr, nullptr);
            return texture;
        }

        ubyte* bm_bits = nullptr;
        ubyte* bm_pal = nullptr;
        xlog::trace("Locking bitmap");
        auto fmt = bm::lock(bm_handle, &bm_bits, &bm_pal);
        if (fmt == bm::FORMAT_NONE || bm_bits == nullptr || bm_pal != nullptr) {
            xlog::warn("Bitmap lock failed or unsupported bitmap");
            return {};
        }

        xlog::trace("Creating normal texture");
        auto texture = create_cpu_texture(bm_handle, fmt, w, h, bm_bits, bm_pal);

        xlog::trace("Unlocking bitmap");
        bm::unlock(bm_handle);

        xlog::trace("Texture created");
        return texture;
    }

    TextureManager::Texture& TextureManager::get_or_load_texture(int bm_handle)
    {
        auto it = texture_cache_.find(bm_handle);
        if (it != texture_cache_.end()) {
            return it->second;
        }

        auto insert_result = texture_cache_.insert({bm_handle, std::move(load_texture(bm_handle))});
        return insert_result.first->second;
    }

    ID3D11ShaderResourceView* TextureManager::lookup_texture(int bm_handle)
    {
        if (bm_handle < 0) {
            return nullptr;
        }
        Texture& texture = get_or_load_texture(bm_handle);
        if (!texture.texture_view) {
            xlog::trace("Creating GPU texture for %s", bm::get_filename(bm_handle));
        }
        return texture.get_or_create_texture_view(device_, device_context_);
    }

    ID3D11RenderTargetView* TextureManager::lookup_render_target(int bm_handle)
    {
        if (bm_handle < 0) {
            return nullptr;
        }
        Texture& texture = get_or_load_texture(bm_handle);
        return texture.render_target_view;
    }

    void TextureManager::save_cache()
    {
        for (auto& e : texture_cache_) {
            Texture& texture = e.second;
            ++texture.save_cache_count;
        }
    }

    void TextureManager::flush_cache(bool force)
    {
        xlog::info("Flushing texture cache");
        if (force) {
            xlog::info("Flushing all textures");
            texture_cache_.clear();
        }
        else {
            auto it = texture_cache_.begin();
            while (it != texture_cache_.end()) {
                Texture& texture = it->second;
                if (texture.save_cache_count > 0) {
                    --texture.save_cache_count;
                }
                else if (texture.ref_count <= 0) {
                    xlog::warn("Flushing texture: handle %d filename %s", texture.bm_handle, bm::get_filename(texture.bm_handle));
                    it = texture_cache_.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }

    void TextureManager::add_ref(int bm_handle)
    {
        page_in(bm_handle);
        auto it = texture_cache_.find(bm_handle);
        if (it != texture_cache_.end()) {
            Texture& texture = it->second;
            ++texture.ref_count;
        }
    }

    void TextureManager::remove_ref(int bm_handle)
    {
        auto it = texture_cache_.find(bm_handle);
        if (it != texture_cache_.end()) {
            Texture& texture = it->second;
            assert(texture.ref_count > 0);
            --texture.ref_count;
            if (texture.ref_count <= 0) {
                xlog::warn("Flushing texture after ref removal: handle %d filename %s", texture.bm_handle, bm::get_filename(texture.bm_handle));
                texture_cache_.erase(it);
            }
        }
    }

    DXGI_FORMAT TextureManager::get_dxgi_format(bm::Format fmt)
    {
        switch (fmt) {
            case bm::FORMAT_565_RGB: return DXGI_FORMAT_B5G6R5_UNORM;
            //case bm::FORMAT_1555_ARGB: return DXGI_FORMAT_B5G5R5A1_UNORM; // does not work in Win7 in VirtualBox
            //case bm::FORMAT_4444_ARGB: return DXGI_FORMAT_B4G4R4A4_UNORM; // does not work in Win7 in VirtualBox
            case bm::FORMAT_8888_ARGB: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case bm::FORMAT_DXT1: return DXGI_FORMAT_BC1_UNORM;
            case bm::FORMAT_DXT3: return DXGI_FORMAT_BC2_UNORM;
            case bm::FORMAT_DXT5: return DXGI_FORMAT_BC3_UNORM;
            default: return DXGI_FORMAT_UNKNOWN;
        }
    }

    rf::bm::Format TextureManager::get_bm_format(DXGI_FORMAT dxgi_fmt)
    {
        switch (dxgi_fmt) {
            case DXGI_FORMAT_B5G6R5_UNORM: return bm::FORMAT_565_RGB;
            case DXGI_FORMAT_B5G5R5A1_UNORM: return bm::FORMAT_1555_ARGB;
            case DXGI_FORMAT_B4G4R4A4_UNORM: return bm::FORMAT_4444_ARGB;
            case DXGI_FORMAT_B8G8R8A8_UNORM: return bm::FORMAT_8888_ARGB;
            case DXGI_FORMAT_BC1_UNORM: return bm::FORMAT_DXT1;
            case DXGI_FORMAT_BC2_UNORM: return bm::FORMAT_DXT3;
            case DXGI_FORMAT_BC3_UNORM: return bm::FORMAT_DXT5;
            default: return bm::FORMAT_NONE;
        }
    }

    static D3D11_MAP convert_lock_mode_to_map_type(gr::LockMode mode)
    {
        switch (mode) {
            case gr::LOCK_READ_ONLY:
                return D3D11_MAP_READ;
            case gr::LOCK_READ_ONLY_WRITE:
                return D3D11_MAP_READ_WRITE;
            case gr::LOCK_WRITE_ONLY:
                return D3D11_MAP_WRITE;
            default:
                xlog::warn("Invalid lock mode: %d", mode);
                assert(false);
                return D3D11_MAP_READ_WRITE;
        }
    }

    bool TextureManager::lock(int bm_handle, int section, gr::LockInfo *lock)
    {
        if (section != 0) {
            return false;
        }

        int w, h;
        bm::get_dimensions(bm_handle, &w, &h);
        if (w == 0 || h == 0) {
            return false;
        }

        Texture& texture = get_or_load_texture(bm_handle);
        if (!texture.cpu_texture) {
            xlog::warn("Attempted to lock texture without cpu resource %d", texture.bm_handle);
            return false;
        }

        D3D11_TEXTURE2D_DESC desc;
        texture.cpu_texture->GetDesc(&desc);

        D3D11_MAPPED_SUBRESOURCE mapped_texture;
        D3D11_MAP map_type = convert_lock_mode_to_map_type(lock->mode);
        HRESULT hr = device_context_->Map(texture.cpu_texture, 0, map_type, 0, &mapped_texture);
        check_hr(hr, "Map texture");

        lock->bm_handle = bm_handle;
        lock->section = section;
        lock->format = get_bm_format(desc.Format);
        lock->data = reinterpret_cast<rf::ubyte*>(mapped_texture.pData);
        lock->w = w;
        lock->h = h;
        lock->stride_in_bytes = mapped_texture.RowPitch;
        // Note: lock->mode is set by gr_lock
        xlog::trace("locked texture: handle %d format %d size %dx%d data %p", lock->bm_handle, lock->format, lock->w, lock->h, lock->data);
        return true;
    }

    void TextureManager::unlock(gr::LockInfo *lock)
    {
        xlog::trace("unlocking texture: handle %d format %d size %dx%d data %p", lock->bm_handle, lock->format, lock->w, lock->h, lock->data);
        Texture& texture = get_or_load_texture(lock->bm_handle);
        device_context_->Unmap(texture.cpu_texture, 0);
        if (lock->mode != rf::gr::LOCK_READ_ONLY && texture.gpu_texture) {
            device_context_->CopySubresourceRegion(texture.gpu_texture, 0, 0, 0, 0, texture.cpu_texture, 0, nullptr);
        }
    }

    void TextureManager::get_texel(
        [[maybe_unused]] int bm_handle,
        [[maybe_unused]] float u,
        [[maybe_unused]] float v,
        rf::gr::Color *clr)
    {
        const auto& it = texture_cache_.find(bm_handle);
        if (it != texture_cache_.end()) {
            Texture& texture = it->second;
            D3D11_MAPPED_SUBRESOURCE mapped_texture;
            HRESULT hr = device_context_->Map(texture.cpu_texture, 0, D3D11_MAP_READ, 0, &mapped_texture);
            check_hr(hr, "Map texture");

            // TODO: convert color

            device_context_->Unmap(texture.cpu_texture, 0);
        }
        clr->set(255, 255, 255, 255);
    }

    void TextureManager::Texture::init_gpu_texture(ID3D11Device* device, ID3D11DeviceContext* device_context)
    {
        if (!gpu_texture) {
            if (!cpu_texture) {
                xlog::warn("Both GPU and CPU textures are missing");
                return;
            }

            D3D11_TEXTURE2D_DESC cpu_desc;
            cpu_texture->GetDesc(&cpu_desc);

            CD3D11_TEXTURE2D_DESC desc{
                cpu_desc.Format,
                cpu_desc.Width,
                cpu_desc.Height,
                1, // arraySize
                1, // mipLevels
            };
            HRESULT hr = device->CreateTexture2D(&desc, nullptr, &gpu_texture);
            check_hr(hr, "CreateTexture2D gpu");
            gpu_texture->GetDesc(&desc);

            // Copy only first level
            // TODO: mapmaps?
            device_context->CopySubresourceRegion(gpu_texture, 0, 0, 0, 0, cpu_texture, 0, nullptr);
        }

        HRESULT hr = device->CreateShaderResourceView(gpu_texture, nullptr, &texture_view);
        check_hr(hr, "CreateShaderResourceView");

        xlog::trace("Created GPU texture");
    }
}
