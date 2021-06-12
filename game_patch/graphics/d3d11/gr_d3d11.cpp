#include <cassert>
#include <unordered_map>
#include <d3d11.h>
#include <patch_common/ComPtr.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../../rf/gr/gr.h"
#include "../../rf/file/file.h"
#include "../../rf/os/os.h"
#include "../../bmpman/bmpman.h"

using namespace rf;

struct GpuVertex
{
    float x;
    float y;
    float z;
    // float w;
    float u0;
    float v0;
    float u1;
    float v1;
    int diffuse;
};

struct PixelShaderConstantBuffer
{
    float ts;
    float cs;
    float as;
    float alpha_test;
};

// constexpr ubyte GR_DIRECT3D11 = 0x7A;

class D3D11Renderer
{
public:
    D3D11Renderer(HWND hwnd, HMODULE d3d11_lib);
    ~D3D11Renderer();
    void window_active();
    void window_inactive();
    void set_mode(gr::Mode mode);
    void set_texture(int slot, int bm_handle);
    void tmapper(int nv, gr::Vertex **vertices, int tmap_flags, gr::Mode mode);
    void bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, gr::Mode mode);
    void clear();
    void zbuffer_clear();
    void set_clip();
    void flip();
    int lock(int bm_handle, int section, gr::LockInfo *lock, int mode);
    void unlock(gr::LockInfo *lock);

private:
    void init_device(HWND hwnd, HMODULE d3d11_lib);
    void init_back_buffer();
    void init_depth_stencil_buffer();
    void create_dynamic_vb();
    void create_dynamic_ib();
    void init_constant_buffer();
    void init_vertex_shader();
    void init_pixel_shader();
    void init_rasterizer_state();
    void map_dynamic_buffers(bool vb_full, bool ib_full);
    void unmap_dynamic_buffers();
    int pack_color(int r, int g, int b, int a);
    DXGI_FORMAT get_dxgi_format(bm::Format fmt);
    ComPtr<ID3D11ShaderResourceView> create_texture(bm::Format fmt, int w, int h, ubyte* bits, ubyte* pal);
    ComPtr<ID3D11ShaderResourceView> create_texture(int bm_handle);
    ID3D11ShaderResourceView* lookup_texture(int bm_handle);
    ID3D11SamplerState* lookup_sampler_state(gr::Mode mode, int slot);
    ID3D11BlendState* lookup_blend_state(gr::Mode mode);
    ID3D11DepthStencilState* lookup_depth_stencil_state(gr::Mode mode);
    void batch_flush();

    ComPtr<ID3D11Device> device_;
    ComPtr<IDXGISwapChain> swap_chain_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11RenderTargetView> back_buffer_view_;
    ComPtr<ID3D11DepthStencilView> depth_stencil_buffer_view_;
    ComPtr<ID3D11Buffer> dynamic_vb_;
    ComPtr<ID3D11Buffer> dynamic_ib_;
    ComPtr<ID3D11Buffer> ps_cbuffer_;
    ComPtr<ID3D11VertexShader> vertex_shader_;
    ComPtr<ID3D11PixelShader> pixel_shader_;
    ComPtr<ID3D11InputLayout> input_layout_;
    std::unordered_map<int, ComPtr<ID3D11ShaderResourceView>> texture_cache_;
    std::unordered_map<int, ComPtr<ID3D11SamplerState>> sampler_state_cache_;
    std::unordered_map<int, ComPtr<ID3D11BlendState>> blend_state_cache_;
    std::unordered_map<int, ComPtr<ID3D11DepthStencilState>> depth_stencil_state_cache_;
    GpuVertex* mapped_vb_;
    ushort* mapped_ib_;
    D3D11_PRIMITIVE_TOPOLOGY batch_primitive_topology_;
    int batch_current_vertex_ = 0;
    int batch_start_index_ = 0;
    int batch_current_index_ = 0;
    int batch_textures_[2] = { -1, -1 };
    gr::Mode batch_mode_{
        gr::TEXTURE_SOURCE_NONE,
        gr::COLOR_SOURCE_VERTEX,
        gr::ALPHA_SOURCE_VERTEX,
        gr::ALPHA_BLEND_NONE,
        gr::ZBUFFER_TYPE_NONE,
        gr::FOG_ALLOWED,
    };
};

static HMODULE d3d11_lib;
static std::optional<D3D11Renderer> d3d11_renderer;

constexpr int d3d11_batch_max_vertex = 6000;
constexpr int d3d11_batch_max_index = 10000;
constexpr float d3d11_zm = 1 / 1700.0f;

static void check_hr(HRESULT hr, const char* fun)
{
    if (FAILED(hr)) {
        xlog::error("%s failed: %lx", fun, hr);
        RF_DEBUG_ERROR("D3D11 subsystem fatal error");
    }
}

void D3D11Renderer::window_active()
{
    if (gr::screen.window_mode == gr::FULLSCREEN) {
        xlog::warn("gr_d3d11_window_active SetFullscreenState");
        HRESULT hr = swap_chain_->SetFullscreenState(TRUE, nullptr);
        check_hr(hr, "SetFullscreenState");
    }
}

void D3D11Renderer::window_inactive()
{
    if (gr::screen.window_mode == gr::FULLSCREEN) {
        xlog::warn("gr_d3d11_window_inactive SetFullscreenState");
        HRESULT hr = swap_chain_->SetFullscreenState(FALSE, nullptr);
        check_hr(hr, "SetFullscreenState");
    }
}

void D3D11Renderer::init_device(HWND hwnd, HMODULE d3d11_lib)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = gr::screen.window_mode == gr::FULLSCREEN ? 2 : 1;
    sd.BufferDesc.Width = gr::screen.max_w;
    sd.BufferDesc.Height = gr::screen.max_h;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    //sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = gr::screen.window_mode == gr::WINDOWED;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

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
    check_hr(hr, "pD3D11CreateDeviceAndSwapChain");

    xlog::info("D3D11 feature level: 0x%x", feature_level_supported);
}

void D3D11Renderer::init_back_buffer()
{
    // Get a pointer to the back buffer
    ComPtr<ID3D11Texture2D> back_buffer;
    HRESULT hr = swap_chain_->GetBuffer(0, IID_ID3D11Texture2D, (LPVOID*)&back_buffer);
    check_hr(hr, "GetBuffer");

    // Create a render-target view
    hr = device_->CreateRenderTargetView(back_buffer, NULL, &back_buffer_view_);
    check_hr(hr, "CreateRenderTargetView");
}

void D3D11Renderer::init_depth_stencil_buffer()
{
    D3D11_TEXTURE2D_DESC depth_stencil_desc;
    ZeroMemory(&depth_stencil_desc, sizeof(depth_stencil_desc));
    depth_stencil_desc.Width = gr::screen.max_w;
    depth_stencil_desc.Height = gr::screen.max_h;
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

void D3D11Renderer::create_dynamic_vb()
{
    // Fill in a buffer description.
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(GpuVertex) * d3d11_batch_max_vertex;
    buffer_desc.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    // Create the vertex buffer.
    HRESULT hr = device_->CreateBuffer(&buffer_desc, nullptr, &dynamic_vb_);
    check_hr(hr, "CreateBuffer");
}

void D3D11Renderer::create_dynamic_ib()
{
    // Fill in a buffer description.
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(ushort) * d3d11_batch_max_index;
    buffer_desc.BindFlags        = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    // Create the vertex buffer.
    HRESULT hr = device_->CreateBuffer(&buffer_desc, nullptr, &dynamic_ib_);
    check_hr(hr, "CreateBuffer");
}

void D3D11Renderer::init_constant_buffer()
{
    D3D11_BUFFER_DESC buffer_desc;
    ZeroMemory(&buffer_desc, sizeof(buffer_desc));
    buffer_desc.Usage            = D3D11_USAGE_DYNAMIC;
    buffer_desc.ByteWidth        = sizeof(PixelShaderConstantBuffer);
    buffer_desc.BindFlags        = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags        = 0;
    buffer_desc.StructureByteStride = 0;

    PixelShaderConstantBuffer data;
    data.ts = 0.0f;
    D3D11_SUBRESOURCE_DATA subres_data;
    subres_data.pSysMem = &data;
    subres_data.SysMemPitch = 0;
    subres_data.SysMemSlicePitch = 0;

    HRESULT hr = device_->CreateBuffer(&buffer_desc, &subres_data, &ps_cbuffer_);
    check_hr(hr, "CreateBuffer");

    ID3D11Buffer* ps_cbuffers[] = { ps_cbuffer_ };
    context_->PSSetConstantBuffers(0, std::size(ps_cbuffers), ps_cbuffers);
}

void D3D11Renderer::init_vertex_shader()
{
    rf::File file;
    if (file.open("default_vs.bin") != 0) {
        xlog::error("Cannot open vertex shader file");
        return;
    }
    int size = file.size();
    auto shader_data = std::make_unique<std::byte[]>(size);
    int bytes_read = file.read(shader_data.get(), size);
    xlog::info("Vertex shader size %d file size %d first byte %02x", bytes_read, size, (ubyte)shader_data.get()[0]);
    HRESULT hr = device_->CreateVertexShader(shader_data.get(), size, nullptr, &vertex_shader_);
    check_hr(hr, "CreateVertexShader");

    D3D11_INPUT_ELEMENT_DESC desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    xlog::info("layout size %d", std::size(desc));
    hr = device_->CreateInputLayout(
        desc,
        std::size(desc),
        shader_data.get(),
        size,
        &input_layout_
    );
    check_hr(hr, "CreateInputLayout");

    context_->IASetInputLayout(input_layout_);
    context_->VSSetShader(vertex_shader_, nullptr, 0);
}

void D3D11Renderer::init_pixel_shader()
{
    rf::File file;
    if (file.open("default_ps.bin") != 0) {
        xlog::error("Cannot open pixel shader file");
        return;
    }
    int size = file.size();
    auto shader_data = std::make_unique<std::byte[]>(size);
    file.read(shader_data.get(), size);
    HRESULT hr = device_->CreatePixelShader(shader_data.get(), size, nullptr, &pixel_shader_);
    check_hr(hr, "CreatePixelShader");

    context_->PSSetShader(pixel_shader_, nullptr, 0);
}

void D3D11Renderer::init_rasterizer_state()
{
    CD3D11_RASTERIZER_DESC desc{CD3D11_DEFAULT{}};
    desc.CullMode = D3D11_CULL_NONE;
    ComPtr<ID3D11RasterizerState> rasterizer_state;
    HRESULT hr = device_->CreateRasterizerState(&desc, &rasterizer_state);
    check_hr(hr, "CreateRasterizerState");
    context_->RSSetState(rasterizer_state);
}

D3D11Renderer::D3D11Renderer(HWND hwnd, HMODULE d3d11_lib)
{
    init_device(hwnd, d3d11_lib);
    init_back_buffer();
    init_depth_stencil_buffer();
    init_rasterizer_state();
    init_vertex_shader();
    init_pixel_shader();
    create_dynamic_vb();
    create_dynamic_ib();
    init_constant_buffer();

    // Bind the view
    ID3D11RenderTargetView* render_targets[] = { back_buffer_view_ };
    context_->OMSetRenderTargets(std::size(render_targets), render_targets, depth_stencil_buffer_view_);

    // TODO: move
    UINT stride = sizeof(GpuVertex);
    UINT offset = 0;
    ID3D11Buffer* vertex_buffers[] = { dynamic_vb_ };
    context_->IASetVertexBuffers(0, std::size(vertex_buffers), vertex_buffers, &stride, &offset);
    context_->IASetIndexBuffer(dynamic_ib_, DXGI_FORMAT_R16_UINT, 0);

    //gr::screen.mode = GR_DIRECT3D11;
    gr::screen.depthbuffer_type = gr::DEPTHBUFFER_Z;
}

D3D11Renderer::~D3D11Renderer()
{
    if (context_) {
        context_->ClearState();
    }
    if (gr::screen.window_mode == gr::FULLSCREEN) {
        swap_chain_->SetFullscreenState(FALSE, nullptr);
    }
}

void D3D11Renderer::map_dynamic_buffers(bool vb_full, bool ib_full)
{
    D3D11_MAPPED_SUBRESOURCE mapped_vb;
    D3D11_MAPPED_SUBRESOURCE mapped_ib;
    ZeroMemory(&mapped_vb, sizeof(D3D11_MAPPED_SUBRESOURCE));
    ZeroMemory(&mapped_ib, sizeof(D3D11_MAPPED_SUBRESOURCE));
    auto vb_map_type = vb_full ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    auto ib_map_type = ib_full ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
    HRESULT hr = context_->Map(dynamic_vb_, 0, vb_map_type, 0, &mapped_vb);
    check_hr(hr, "Map VB");
    hr = context_->Map(dynamic_ib_, 0, ib_map_type, 0, &mapped_ib);
    check_hr(hr, "Map IB");
    mapped_vb_ = reinterpret_cast<GpuVertex*>(mapped_vb.pData);
    mapped_ib_ = reinterpret_cast<ushort*>(mapped_ib.pData);
    if (vb_full) {
        batch_current_vertex_ = 0;
    }
    if (ib_full) {
        batch_current_index_ = 0;
        batch_start_index_ = 0;
    }
}

void D3D11Renderer::unmap_dynamic_buffers()
{
    context_->Unmap(dynamic_vb_, 0);
    context_->Unmap(dynamic_ib_, 0);
    mapped_vb_ = nullptr;
    mapped_ib_ = nullptr;
}

inline int D3D11Renderer::pack_color(int r, int g, int b, int a)
{
    // assume little endian
    return (a << 24) | (b << 16) | (g << 8) | r;
}

DXGI_FORMAT D3D11Renderer::get_dxgi_format(bm::Format fmt)
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

ComPtr<ID3D11ShaderResourceView> D3D11Renderer::create_texture(bm::Format fmt, int w, int h, ubyte* bits, ubyte* pal)
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

ComPtr<ID3D11ShaderResourceView> D3D11Renderer::create_texture(int bm_handle)
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

ID3D11ShaderResourceView* D3D11Renderer::lookup_texture(int bm_handle)
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

ID3D11SamplerState* D3D11Renderer::lookup_sampler_state(gr::Mode mode, int slot)
{
    auto ts = mode.get_texture_source();
    if (ts == gr::TEXTURE_SOURCE_NONE) {
        return nullptr;
    }

    if (slot == 1) {
        ts = gr::TEXTURE_SOURCE_CLAMP;
    }

    int cache_key = static_cast<int>(ts);
    const auto& it = sampler_state_cache_.find(cache_key);
    if (it != sampler_state_cache_.end()) {
        return it->second;
    }

    CD3D11_SAMPLER_DESC desc{CD3D11_DEFAULT()};
    switch (ts) {
        case gr::TEXTURE_SOURCE_WRAP:
        case gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0:
        case gr::TEXTURE_SOURCE_CLAMP_1_WRAP_0_MOD2X:
        case gr::TEXTURE_SOURCE_MT_WRAP_TRILIN: // WRAP_1_WRAP_0
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        case gr::TEXTURE_SOURCE_CLAMP:
        case gr::TEXTURE_SOURCE_CLAMP_1_CLAMP_0:
        case gr::TEXTURE_SOURCE_MT_CLAMP_TRILIN: // WRAP_1_CLAMP_0
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case gr::TEXTURE_SOURCE_CLAMP_NO_FILTERING:
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case gr::TEXTURE_SOURCE_MT_U_WRAP_V_CLAMP:
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            break;
        case gr::TEXTURE_SOURCE_MT_U_CLAMP_V_WRAP:
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            break;
        default:
            break;
    }

    ComPtr<ID3D11SamplerState> sampler_state;
    HRESULT hr = device_->CreateSamplerState(&desc, &sampler_state);
    check_hr(hr, "CreateSamplerState");

    sampler_state_cache_.insert({cache_key, sampler_state});
    return sampler_state;
}

ID3D11BlendState* D3D11Renderer::lookup_blend_state(gr::Mode mode)
{
    auto ab = mode.get_alpha_blend();
    int cache_key = static_cast<int>(ab);

    const auto& it = blend_state_cache_.find(cache_key);
    if (it != blend_state_cache_.end()) {
        return it->second;
    }
    CD3D11_BLEND_DESC desc{CD3D11_DEFAULT()};

    switch (ab) {
    case gr::ALPHA_BLEND_NONE:
        desc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case gr::ALPHA_BLEND_ADDITIVE:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        break;
    case gr::ALPHA_BLEND_ALPHA_ADDITIVE:
    case gr::ALPHA_BLEND_SRC_COLOR:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        break;
    case gr::ALPHA_BLEND_ALPHA:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        break;
    case gr::ALPHA_BLEND_LIGHTMAP:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        break;
    case gr::ALPHA_BLEND_SUBTRACTIVE:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        break;
    case gr::ALPHA_BLEND_SWAPPED_SRC_DEST_COLOR:
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_DEST_COLOR;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_COLOR;
        break;
    }

    ComPtr<ID3D11BlendState> blend_state;
    HRESULT hr = device_->CreateBlendState(&desc, &blend_state);
    check_hr(hr, "CreateBlendState");

    blend_state_cache_.insert({cache_key, blend_state});
    return blend_state;
}

ID3D11DepthStencilState* D3D11Renderer::lookup_depth_stencil_state(gr::Mode mode)
{
    auto zbt = mode.get_zbuffer_type();
    int cache_key = static_cast<int>(zbt) | (static_cast<int>(gr::screen.depthbuffer_type) << 8);

    const auto& it = depth_stencil_state_cache_.find(cache_key);
    if (it != depth_stencil_state_cache_.end()) {
        return it->second;
    }
    CD3D11_DEPTH_STENCIL_DESC desc{CD3D11_DEFAULT()};

    if (gr::screen.depthbuffer_type == gr::DEPTHBUFFER_Z) {
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    }
    else {
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    }
    switch (zbt) {
    case gr::ZBUFFER_TYPE_NONE:
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        break;
    case gr::ZBUFFER_TYPE_READ:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        break;
    case gr::ZBUFFER_TYPE_READ_EQUAL:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_EQUAL;
        break;
    case gr::ZBUFFER_TYPE_WRITE:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        break;
    case gr::ZBUFFER_TYPE_FULL:
    case gr::ZBUFFER_TYPE_FULL_ALPHA_TEST:
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        break;
    }

    ComPtr<ID3D11DepthStencilState> depth_stencil_state;
    HRESULT hr = device_->CreateDepthStencilState(&desc, &depth_stencil_state);
    check_hr(hr, "CreateDepthStencilState");

    depth_stencil_state_cache_.insert({cache_key, depth_stencil_state});
    return depth_stencil_state;
}

void D3D11Renderer::set_mode(gr::Mode mode)
{
    D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
    HRESULT hr = context_->Map(ps_cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
    check_hr(hr, "Map");

    PixelShaderConstantBuffer& ps_data = *reinterpret_cast<PixelShaderConstantBuffer*>(mapped_cbuffer.pData);
    ps_data.ts = mode.get_texture_source();
    ps_data.cs = mode.get_color_source();
    ps_data.as = mode.get_alpha_source();
    bool alpha_test = mode.get_zbuffer_type() == gr::ZBUFFER_TYPE_FULL_ALPHA_TEST;
    ps_data.alpha_test = alpha_test ? 1.0f : 0.0f;
    context_->Unmap(ps_cbuffer_, 0);

    ID3D11SamplerState* sampler_states[] = {
        lookup_sampler_state(mode, 0),
        lookup_sampler_state(mode, 1),
    };
    context_->PSSetSamplers(0, std::size(sampler_states), sampler_states);

    ID3D11BlendState* blend_state = lookup_blend_state(mode);
    context_->OMSetBlendState(blend_state, nullptr, 0xffffffff);

    ID3D11DepthStencilState* depth_stencil_state = lookup_depth_stencil_state(mode);
    context_->OMSetDepthStencilState(depth_stencil_state, 0);

    // if (get_texture_source(mode) == TEXTURE_SOURCE_NONE) {
    //     gr::screen.current_bitmap = -1;
    // }
}

void D3D11Renderer::set_texture(int slot, int bm_handle)
{
    ID3D11ShaderResourceView* shader_resources[] = { lookup_texture(bm_handle) };
    context_->PSSetShaderResources(slot, std::size(shader_resources), shader_resources);
}

void D3D11Renderer::batch_flush()
{
    int num_index = batch_current_index_ - batch_start_index_;
    if (num_index == 0) {
        return;
    }

    if (mapped_vb_) {
        unmap_dynamic_buffers();
    }

    context_->IASetPrimitiveTopology(batch_primitive_topology_);
    set_mode(batch_mode_);
    set_texture(0, batch_textures_[0]);
    set_texture(1, batch_textures_[1]);

    context_->DrawIndexed(num_index, batch_start_index_, 0);
    batch_start_index_ = batch_current_index_;
}

void D3D11Renderer::tmapper(int nv, gr::Vertex **vertices, int tmap_flags, gr::Mode mode)
{
    if (mode.get_zbuffer_type() == gr::ZBUFFER_TYPE_FULL) {
        //xlog::info("gr_d3d11_tmapper nv %d sx %f sy %f sw %f", nv, vertices[0]->sx, vertices[0]->sy, vertices[0]->sw);
        //mode.set_texture_source(TEXTURE_SOURCE_NONE);
        //gr::screen.current_color.alpha = 255;
    }
    //
    //gr_d3d11_set_mode(mode);

    int num_index = (nv - 2) * 3;
    if (nv > d3d11_batch_max_vertex || num_index > d3d11_batch_max_index) {
        xlog::error("too many vertices/indices in gr_d3d11_tmapper");
        return;
    }
    bool vb_full = batch_current_vertex_ + nv >= d3d11_batch_max_vertex;
    bool ib_full = batch_current_index_ + num_index > d3d11_batch_max_index;
    bool mode_changed = batch_mode_ != mode;
    bool texture_changed = batch_textures_[0] != gr::screen.current_texture_1 || batch_textures_[1] != gr::screen.current_texture_2;
    bool topology_changed = batch_primitive_topology_ != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    if (vb_full || ib_full || topology_changed || mode_changed || texture_changed) {
        batch_flush();
        batch_mode_ = mode;
        batch_textures_[0] = gr::screen.current_texture_1;
        batch_textures_[1] = gr::screen.current_texture_2;
        batch_primitive_topology_ = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
    if (!mapped_vb_) {
        map_dynamic_buffers(vb_full, ib_full);
    }

    int r = 255, g = 255, b = 255, a = 255;
    if (!(tmap_flags & gr::TMAP_FLAG_RGB)) {
        r = gr::screen.current_color.red;
        g = gr::screen.current_color.green;
        b = gr::screen.current_color.blue;
    }
    if (!(tmap_flags & gr::TMAP_FLAG_ALPHA)) {
        a = gr::screen.current_color.alpha;
    }
    int vert0 = batch_current_vertex_;
    for (int i = 0; i < nv; ++i) {
        auto& in_vert = *vertices[i];
        auto& out_vert = mapped_vb_[batch_current_vertex_];
        out_vert.x = (in_vert.sx - gr::screen.offset_x) / gr::screen.clip_width * 2.0f - 1.0f;
        out_vert.y = (in_vert.sy - gr::screen.offset_y) / gr::screen.clip_height * -2.0f + 1.0f;
        out_vert.z = in_vert.sw * d3d11_zm;
        if (tmap_flags & gr::TMAP_FLAG_RGB) {
            r = in_vert.r;
            g = in_vert.g;
            b = in_vert.b;
        }
        if (tmap_flags & gr::TMAP_FLAG_ALPHA) {
            a = in_vert.a;
        }
        out_vert.diffuse = pack_color(r, g, b, a);
        out_vert.u0 = in_vert.u1;
        out_vert.v0 = in_vert.v1;
        // out_vert.u1 = in_vert.u2;
        // out_vert.v1 = in_vert.v2;
        //xlog::info("vert[%d]: pos <%.2f %.2f %.2f> uv <%.2f %.2f>", i, out_vert.x, out_vert.y, in_vert.sw, out_vert.u0, out_vert.v0);

        if (i >= 2) {
            mapped_ib_[batch_current_index_++] = vert0;
            mapped_ib_[batch_current_index_++] = batch_current_vertex_ - 1;
            mapped_ib_[batch_current_index_++] = batch_current_vertex_;
        }
        ++batch_current_vertex_;
    }
}

void D3D11Renderer::bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, gr::Mode mode)
{
    //xlog::info("gr_d3d11_bitmap");
    //gr_d3d11_set_texture(0, bitmap_handle);
    gr::screen.current_texture_1 = bitmap_handle;
    int bm_w, bm_h;
    bm::get_dimensions(bitmap_handle, &bm_w, &bm_h);
    gr::Vertex verts[4];
    gr::Vertex* verts_ptrs[] = {
        &verts[0],
        &verts[1],
        &verts[2],
        &verts[3],
    };
    float sx_left = gr::screen.offset_x + x;
    float sx_right = gr::screen.offset_x + x + w;
    float sy_top = gr::screen.offset_y + y;
    float sy_bottom = gr::screen.offset_y + y + h;
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
    tmapper(std::size(verts_ptrs), verts_ptrs, 0, mode);
}

void D3D11Renderer::clear()
{
    float clear_color[4] = {
        gr::screen.current_color.red / 255.0f,
        gr::screen.current_color.green / 255.0f,
        gr::screen.current_color.blue / 255.0f,
        1.0f,
    };
    context_->ClearRenderTargetView(back_buffer_view_, clear_color);
}

void D3D11Renderer::zbuffer_clear()
{
    if (gr::screen.depthbuffer_type != gr::DEPTHBUFFER_NONE) {
        float depth = gr::screen.depthbuffer_type == gr::DEPTHBUFFER_Z ? 0.0f : 1.0f;
        context_->ClearDepthStencilView(depth_stencil_buffer_view_, D3D11_CLEAR_DEPTH, depth, 0);
    }
}

void D3D11Renderer::set_clip()
{
    batch_flush();
    D3D11_VIEWPORT vp;
    vp.TopLeftX = gr::screen.clip_left + gr::screen.offset_x;
    vp.TopLeftY = gr::screen.clip_top + gr::screen.offset_y;
    vp.Width = gr::screen.clip_width;
    vp.Height = gr::screen.clip_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &vp);
}

void D3D11Renderer::flip()
{
    //xlog::info("gr_d3d11_flip");
    batch_flush();
    HRESULT hr = swap_chain_->Present(0, 0);
    check_hr(hr, "Present");
}

int D3D11Renderer::lock(int bm_handle, int section, gr::LockInfo *lock, int mode)
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

void D3D11Renderer::unlock(gr::LockInfo *lock)
{
    //xlog::info("unlocking bm %d format %d size %dx%d data %p", lock->bm_handle, lock->format, lock->w, lock->h, lock->data);
    auto texture_view = create_texture(lock->format, lock->w, lock->h, lock->data, nullptr);
    delete[] lock->data;
    lock->data = nullptr;
    texture_cache_.insert({lock->bm_handle, texture_view});
}

void gr_d3d11_msg_handler(UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_ACTIVATEAPP:
        if (w_param) {
            xlog::warn("active %x %lx", w_param, l_param);
            d3d11_renderer->window_active();
        }
        else {
            xlog::warn("inactive %x %lx", w_param, l_param);
            d3d11_renderer->window_inactive();
        }
    }
}

void gr_d3d11_flip()
{
    d3d11_renderer->flip();
}

void gr_d3d11_close()
{
    xlog::info("gr_d3d11_close");
    d3d11_renderer.reset();
    FreeLibrary(d3d11_lib);
}

void gr_d3d11_init(HWND hwnd)
{
    xlog::info("gr_d3d11_init");
    d3d11_lib = LoadLibraryW(L"d3d11.dll");
    if (!d3d11_lib) {
        RF_DEBUG_ERROR("Failed to load d3d11.dll");
    }

    d3d11_renderer.emplace(hwnd, d3d11_lib);
    os_add_msg_handler(gr_d3d11_msg_handler);
}

void gr_d3d11_clear()
{
    d3d11_renderer->clear();
}

void gr_d3d11_bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, gr::Mode mode)
{
    d3d11_renderer->bitmap(bitmap_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
}

void gr_d3d11_set_clip()
{
    d3d11_renderer->set_clip();
}

void gr_d3d11_zbuffer_clear()
{
    d3d11_renderer->zbuffer_clear();
}

void gr_d3d11_tmapper(int nv, gr::Vertex **vertices, int tmap_flags, gr::Mode mode)
{
    d3d11_renderer->tmapper(nv, vertices, tmap_flags, mode);
}

int gr_d3d11_lock(int bm_handle, int section, gr::LockInfo *lock, int mode)
{
    return d3d11_renderer->lock(bm_handle, section, lock, mode);
}

void gr_d3d11_unlock(gr::LockInfo *lock)
{
    d3d11_renderer->unlock(lock);
}

void gr_d3d11_apply_patch()
{
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
    AsmWriter{0x00546730}.ret(); // gr_d3d_read_backbuffer
    AsmWriter{0x005468C0}.ret(); // gr_d3d_set_fog
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
    AsmWriter{0x00550820}.ret(); // gr_d3d_page_in
    AsmWriter{0x005508C0}.jmp(gr_d3d11_clear); // gr_d3d_clear
    AsmWriter{0x00550980}.jmp(gr_d3d11_zbuffer_clear); // gr_d3d_zbuffer_clear
    AsmWriter{0x00550A30}.jmp(gr_d3d11_set_clip); // gr_d3d_set_clip
    AsmWriter{0x00550AA0}.jmp(gr_d3d11_bitmap); // gr_d3d_bitmap
    AsmWriter{0x00551450}.ret(); // gr_d3d_flush_after_color_change
    AsmWriter{0x00551460}.ret(); // gr_d3d_line
    AsmWriter{0x00551900}.jmp(gr_d3d11_tmapper); // gr_d3d_tmapper
    //AsmWriter{0x00553C60}.ret(); // gr_d3d_render_movable_solid - uses gr_d3d_render_face_list
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
    AsmWriter{0x0055B520}.ret(); // gr_d3d_texture_save_cache
    AsmWriter{0x0055B550}.ret(); // gr_d3d_texture_flush_cache
    AsmWriter{0x0055CDC0}.ret(); // gr_d3d_mark_texture_dirty
    AsmWriter{0x0055CE00}.jmp(gr_d3d11_lock); // gr_d3d_lock
    AsmWriter{0x0055CF60}.jmp(gr_d3d11_unlock); // gr_d3d_unlock
    AsmWriter{0x0055CFA0}.ret(); // gr_d3d_get_texel
    AsmWriter{0x0055D160}.ret(); // gr_d3d_texture_add_ref
    AsmWriter{0x0055D190}.ret(); // gr_d3d_texture_remove_ref
    AsmWriter{0x0055F5E0}.ret(); // gr_d3d_render_static_solid
    AsmWriter{0x00561650}.ret(); // gr_d3d_render_face_list
    AsmWriter{0x0052FA40}.ret(); // gr_d3d_render_vif_mesh
}
