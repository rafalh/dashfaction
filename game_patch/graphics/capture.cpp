#include <windows.h>
#include <d3d8.h>
#include "capture.h"
#include "gr_color.h"
#include "graphics_internal.h"
#include "../main.h"
#include "../rf/graphics.h"
#include "../rf/object.h"
#include "../rf/player.h"
#include "../rf/misc.h"
#include "../rf/file.h"
#include "../utils/com-utils.h"
#include "../utils/list-utils.h"
#include "../utils/string-utils.h"
#include <cstddef>
#include <cstring>
#include <common/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ComPtr.h>

const char g_screenshot_dir_name[] = "screenshots";

static ComPtr<IDirect3DSurface8> g_capture_tmp_surface;
static ComPtr<IDirect3DSurface8> g_depth_stencil_surface;
static std::unique_ptr<byte* []> g_screenshot_scanlines_buf;
static int g_screenshot_dir_id;

namespace rf
{
struct Monitor
{
    struct Monitor *next;
    struct Monitor *prev;
    int monitor_state;
    int clutter_handle;
    int current_camera_handle;
    int bitmap;
    VArray<int> cameras_handles_array;
    Timestamp camera_cycle_timestamp;
    float cycle_delay;
    int gap2C;
    int width;
    int height;
    int flags;
};
static_assert(sizeof(Monitor) == 0x3C);

static auto& GrD3DGetBitmapTexture = AddrAsRef<IDirect3DTexture8*(int bm_handle)>(0x0055D1E0);
static auto& monitor_list = AddrAsRef<Monitor>(0x005C98A8);
} // namespace rf


bool g_render_to_texture_active = false;
ComPtr<IDirect3DSurface8> g_orig_render_target;
ComPtr<IDirect3DSurface8> g_orig_depth_stencil_surface;

IDirect3DSurface8* GetCachedDepthStencilSurface()
{
    if (!g_depth_stencil_surface) {
        auto hr = rf::gr_d3d_device->CreateDepthStencilSurface(
            rf::gr_d3d_pp.BackBufferWidth, rf::gr_d3d_pp.BackBufferHeight, rf::gr_d3d_pp.AutoDepthStencilFormat,
            D3DMULTISAMPLE_NONE, &g_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::CreateDepthStencilSurface failed 0x%lX", hr);
            return nullptr;
        }
    }
    return g_depth_stencil_surface;
}

bool StartRenderToTexture(int bmh)
{
    // Note: texture reference counter is not increased here so ComPtr is not used
    IDirect3DTexture8* d3d_tex = rf::GrD3DGetBitmapTexture(bmh);
    if (!d3d_tex) {
        WARN_ONCE("Bitmap without D3D texture provided in StartRenderToTexture");
        return false;
    }

    ComPtr<IDirect3DSurface8> orig_render_target;
    ComPtr<IDirect3DSurface8> orig_depth_stencil_surface;

    if (!g_render_to_texture_active) {
        auto hr = rf::gr_d3d_device->GetRenderTarget(&orig_render_target);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetRenderTarget failed 0x%lX", hr);
            return false;
        }

        hr = rf::gr_d3d_device->GetDepthStencilSurface(&orig_depth_stencil_surface);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetDepthStencilSurface failed 0x%lX", hr);
            return false;
        }
    }

    ComPtr<IDirect3DSurface8> tex_surface;
    auto hr = d3d_tex->GetSurfaceLevel(0, &tex_surface);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DTexture8::GetSurfaceLevel failed 0x%lX", hr);
        return false;
    }

    auto depth_stencil = GetCachedDepthStencilSurface();
    hr = rf::gr_d3d_device->SetRenderTarget(tex_surface, depth_stencil);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DDevice8::SetRenderTarget failed 0x%lX", hr);
        return false;
    }

    if (!g_render_to_texture_active) {
        g_orig_render_target = orig_render_target;
        g_orig_depth_stencil_surface = orig_depth_stencil_surface;
        g_render_to_texture_active = true;
    }
    return true;
}

void EndRenderToTexture()
{
    if (!g_render_to_texture_active) {
        return;
    }
    auto hr = rf::gr_d3d_device->SetRenderTarget(g_orig_render_target, g_orig_depth_stencil_surface);
    if (FAILED(hr)) {
        ERR_ONCE("IDirect3DDevice8::SetRenderTarget failed 0x%lX", hr);
    }
    g_orig_render_target.release();
    g_orig_depth_stencil_surface.release();
    g_render_to_texture_active = false;
}

#if !D3D_LOCKABLE_BACKBUFFER
CallHook<rf::BmFormat(int, int, int, int, std::byte*)> GrD3DReadBackBuffer_hook{
    0x0050E015,
    [](int x, int y, int width, int height, std::byte* buffer) {
        rf::GrD3DFlushBuffers();

        ComPtr<IDirect3DSurface8> back_buffer;
        HRESULT hr = rf::gr_d3d_device->GetRenderTarget(&back_buffer);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::GetRenderTarget failed 0x%lX", hr);
            return rf::BM_FORMAT_INVALID;
        }

        D3DSURFACE_DESC desc;
        hr = back_buffer->GetDesc(&desc);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DSurface8::GetDesc failed 0x%lX", hr);
            return rf::BM_FORMAT_INVALID;
        }

        // function is sometimes called with all parameters set to 0 to get backbuffer format
        rf::BmFormat pixel_fmt = GetPixelFormatFromD3DFormat(desc.Format);
        if (width == 0 || height == 0) {
            return pixel_fmt;
        }

        // According to Windows performance tests surface created with CreateImageSurface is faster than one from
        // CreateRenderTarget or CreateTexture.
        // Note: it can be slower during resource allocation so create it once
        if (!g_capture_tmp_surface) {
            hr = rf::gr_d3d_device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &g_capture_tmp_surface);
            if (FAILED(hr)) {
                ERR_ONCE("IDirect3DDevice8::CreateImageSurface failed 0x%lX", hr);
                return rf::BM_FORMAT_INVALID;
            }
        }

        RECT src_rect;
        POINT dst_pt{x, y};
        SetRect(&src_rect, x, y, x + width - 1, y + height - 1);

        hr = rf::gr_d3d_device->CopyRects(back_buffer, &src_rect, 1, g_capture_tmp_surface, &dst_pt);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DDevice8::CopyRects failed 0x%lX", hr);
            return rf::BM_FORMAT_INVALID;
        }

        // Note: locking fragment of Render Target fails
        D3DLOCKED_RECT locked_rect;
        hr = g_capture_tmp_surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);
        if (FAILED(hr)) {
            ERR_ONCE("IDirect3DSurface8::LockRect failed 0x%lX (%s)", hr, getDxErrorStr(hr));
            return rf::BM_FORMAT_INVALID;
        }

        int bytes_per_pixel = GetPixelFormatSize(pixel_fmt);
        std::byte* src_ptr =
            reinterpret_cast<std::byte*>(locked_rect.pBits) + y * locked_rect.Pitch + x * bytes_per_pixel;
        std::byte* dst_ptr = buffer;

        for (int i = 0; i < height; ++i) {
            std::memcpy(dst_ptr, src_ptr, width * bytes_per_pixel);
            src_ptr += locked_rect.Pitch;
            dst_ptr += width * bytes_per_pixel;
        }
        g_capture_tmp_surface->UnlockRect();

        xlog::trace("GrReadBackBufferHook (%d %d %d %d) returns %d", x, y, width, height, pixel_fmt);
        return pixel_fmt;
    },
};
#endif // D3D_LOCKABLE_BACKBUFFER

FunHook<void(int, int, int, int, int)> GrCaptureBackBuffer_hook{
    0x0050E4F0,
    [](int x, int y, int width, int height, int bm_handle) {
        if (g_render_to_texture_active) {
            // Nothing to do because we render directly to texture
            if (rf::gr_screen.mode == rf::GR_DIRECT3D) {
                rf::GrD3DFlushBuffers();
            }
        } else {
            GrCaptureBackBuffer_hook.CallTarget(x, y, width, height, bm_handle);
        }
    },
};

CallHook<int(rf::BmFormat, int, int)> bm_create_user_bitmap_monitor_hook{
    0x00412547,
    []([[ maybe_unused ]] rf::BmFormat pixel_fmt, int w, int h) {
        return bm_create_user_bitmap_monitor_hook.CallTarget(rf::BM_FORMAT_RENDER_TARGET, w, h);
    },
};

void MakeSureMonitorBitmapIsDynamic(rf::Monitor& mon)
{
    if (rf::BmGetFormat(mon.bitmap) == rf::BM_FORMAT_RENDER_TARGET) {
        xlog::trace("Changing pixel format for monitor bitmap");
        ChangeUserBitmapPixelFormat(mon.bitmap, rf::BM_FORMAT_RGB_888, true);
    }
}

namespace rf {

struct MeshMaterial
{
    int field_0;
    int flags;
    char has_flag_2_8;
    Color field_9;
    int texture;
    char name[100];
    int field_78;
    int field_7C;
    int field_80;
    float mat_field_24;
    float mat_field_28;
    float ref_cof;
    char ref_map_name[36];
    int ref_tex;
    int field_B8;
    float *mat_field_20;
    int field_C0;
    int field_C4;
};
static_assert(sizeof(MeshMaterial) == 0xC8);

static auto& AnimMeshGetMaterialsArray = AddrAsRef<void(VMesh *vmesh, int *num_materials_out, MeshMaterial **materials_array_out)>(0x00503650);
}

FunHook<void(rf::Monitor&)> MonitorRenderOffState_hook{
    0x00412410,
    [](rf::Monitor& mon) {
        // monitor is no longer displaying view from camera so its texture usage must be changed
        // from render target to dynamic texture
        MakeSureMonitorBitmapIsDynamic(mon);
        MonitorRenderOffState_hook.CallTarget(mon);
    },
};

#if 1
inline bool GenerateStaticNoise()
{
    static int noise_buf;
    static int counter = 0;
    if (counter == 0)
        noise_buf = std::rand();
    bool white = noise_buf & 1;
    noise_buf >>= 1;
    // assume rand() returns 15-bit long random number
    counter = (counter + 1) % 15;
    return white;
}

FunHook<void(rf::Monitor&)> MonitorRenderNoise_hook{
    0x00412370,
    [](rf::Monitor& mon) {
        // No longer use render target texture
        MakeSureMonitorBitmapIsDynamic(mon);
        // Use custom noise generation algohritm because the default one is not uniform enough in high resolution
        rf::GrLockInfo lock;
        if (rf::GrLock(mon.bitmap, 0, &lock, 0)) {
            auto pixel_size = GetPixelFormatSize(lock.pixel_format);
            for (int y = 0; y < lock.height; ++y) {
                auto ptr = lock.bits + y * lock.pitch;
                for (int x = 0; x < lock.width; ++x) {
                    bool white = GenerateStaticNoise();
                    // support 32-bit textures
                    if (pixel_size == 4) {
                        *reinterpret_cast<int32_t*>(ptr) = white ? 0 : -1;
                    }
                    else {
                        *reinterpret_cast<int16_t*>(ptr) = white ? 0 : -1;
                    }
                    ptr += pixel_size;
                }
            }
            rf::GrUnlock(&lock);
        }
    },
};
#else
void ReplaceMonitorScreenBitmap(rf::Monitor& mon, int hbm)
{
    auto clutter = rf::ObjFromHandle(mon.clutter_handle);
    auto vmesh = clutter->vmesh;
    int num_materials;
    rf::MeshMaterial *materials;
    rf::AnimMeshGetMaterialsArray(vmesh, &num_materials, &materials);
    for (int i = 0; i < num_materials; ++i) {
        auto& mat = materials[i];
        if (!_strcmpi(mat.name, "screen.tga")) {
            mat.texture = hbm;
            *mat.mat_field_20 = 0.0f;
        }
    }
}

FunHook<void(rf::Monitor&)> MonitorRenderNoise_hook{
    0x00412370,
    [](rf::Monitor& mon) {
        // monitor is no longer displaying view from camera so its texture usage must be changed
        // from render target to dynamic texture
        //MakeSureMonitorBitmapIsDynamic(mon);
        if (!(mon.flags & 0x1000)) {
            // very good idea but needs more work on UVs...
            auto hbm = rf::BmLoad("gls_noise01.tga", -1, true);
            ReplaceMonitorScreenBitmap(mon, hbm);
            mon.flags |= 0x1000;
        }
        //MonitorRenderNoise_hook.CallTarget(mon);
    },
};
#endif

CodeInjection monitor_render_from_camera_start_render_to_texture{
    0x00412860,
    [](auto& regs) {
        auto mon = reinterpret_cast<rf::Monitor*>(regs.edi);
        StartRenderToTexture(mon->bitmap);
    },
};

CodeInjection railgun_scanner_start_render_to_texture{
    0x004ADD0A,
    [](auto& regs) {
        auto player = reinterpret_cast<rf::Player*>(regs.ebx);
        StartRenderToTexture(player->ir_data.ir_bitmap_handle);
    },
};

CodeInjection rocket_launcher_start_render_to_texture{
    0x004AF0BC,
    [](auto& regs) {
        auto player = reinterpret_cast<rf::Player*>(regs.esi);
        StartRenderToTexture(player->ir_data.ir_bitmap_handle);
    },
};

void RefreshAllMonitors()
{
    auto mon_list = DoublyLinkedList{rf::monitor_list};
    for (auto& mon : mon_list) {
        // force rerender of this monitor in case it is not updated every frame
        // this is needed because DF uses default pool texture
        mon.flags &= 0xFB;
    }
}

CodeInjection d3d_device_lost_patch{
    0x00545042,
    []() {
        xlog::trace("D3D device lost");
        g_depth_stencil_surface.release();
        // Note: g_capture_tmp_surface is in D3DPOOL_SYSTEMMEM so no need to release here
        ReleaseAllDefaultPoolTextures();
        RefreshAllMonitors();
    },
};

CodeInjection d3d_cleanup_patch{
    0x0054527A,
    []() {
        g_depth_stencil_surface.release();
        g_capture_tmp_surface.release();
    },
};

CodeInjection after_game_render_to_dynamic_textures{
    0x00431890,
    []() {
        // Render to back-buffer from this point
        EndRenderToTexture();
    },
};

CodeInjection screenshot_scanlines_array_overflow_fix1{
    0x0055A066,
    [](auto& regs) {
        g_screenshot_scanlines_buf = std::make_unique<byte* []>(rf::gr_screen.max_height);
        regs.ecx = reinterpret_cast<int32_t>(g_screenshot_scanlines_buf.get());
        regs.eip = 0x0055A06D;
    },
};

CodeInjection screenshot_scanlines_array_overflow_fix2{
    0x0055A0DF,
    [](auto& regs) {
        regs.eax = reinterpret_cast<int32_t>(g_screenshot_scanlines_buf.get());
        regs.eip = 0x0055A0E6;
    },
};

void GraphicsCaptureInit()
{
#if !D3D_LOCKABLE_BACKBUFFER
    /* Override default because IDirect3DSurface8::LockRect fails on multisampled back-buffer */
    GrD3DReadBackBuffer_hook.Install();
#endif

    // Use fast GrCaptureBackBuffer implementation which copies backbuffer to texture without copying from VRAM to RAM
    GrCaptureBackBuffer_hook.Install();
    d3d_device_lost_patch.Install();
    d3d_cleanup_patch.Install();
    after_game_render_to_dynamic_textures.Install();

    // Make sure bitmaps used together with GrCaptureBackBuffer have the same format as backbuffer
    bm_create_user_bitmap_monitor_hook.Install();
    MonitorRenderOffState_hook.Install();
    MonitorRenderNoise_hook.Install();

    // Override screenshot directory
    WriteMemPtr(0x004367CA + 2, &g_screenshot_dir_id);

    // Fix buffer overflow in screenshot to JPG conversion code
    screenshot_scanlines_array_overflow_fix1.Install();
    screenshot_scanlines_array_overflow_fix2.Install();

    monitor_render_from_camera_start_render_to_texture.Install();
    railgun_scanner_start_render_to_texture.Install();
    rocket_launcher_start_render_to_texture.Install();

    // Make sure scanner bitmap is a render target
    WriteMem<u8>(0x004A34BF + 1, rf::BM_FORMAT_RENDER_TARGET);
}

void GraphicsCaptureAfterGameInit()
{
    auto full_path = StringFormat("%s\\%s", rf::root_path, g_screenshot_dir_name);
    if (CreateDirectoryA(full_path.c_str(), nullptr))
        xlog::info("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS)
        xlog::error("Failed to create screenshots directory %lu", GetLastError());
    g_screenshot_dir_id = rf::FileAddDirectoryEx(g_screenshot_dir_name, "", true);
}
