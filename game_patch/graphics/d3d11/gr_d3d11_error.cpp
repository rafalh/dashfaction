#include <common/error/error-utils.h>
#include "gr_d3d11.h"

namespace df::gr::d3d11
{
    #define GR_D3D11_ERROR_CASE(hr) case hr: return #hr;

    static ID3D11Device* g_device;

    static const char* get_hresult_code_name(HRESULT hr)
    {
        switch (hr) {
            GR_D3D11_ERROR_CASE(S_OK)
            GR_D3D11_ERROR_CASE(E_FAIL)
            GR_D3D11_ERROR_CASE(E_INVALIDARG)
            GR_D3D11_ERROR_CASE(E_OUTOFMEMORY)
            GR_D3D11_ERROR_CASE(E_NOTIMPL)
            GR_D3D11_ERROR_CASE(S_FALSE)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_ACCESS_DENIED)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_ACCESS_LOST)
            //GR_D3D11_ERROR_CASE(DXGI_ERROR_ALREADY_EXISTS)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_CANNOT_PROTECT_CONTENT)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_DEVICE_HUNG)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_DEVICE_REMOVED)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_DEVICE_RESET)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_FRAME_STATISTICS_DISJOINT)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_INVALID_CALL)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_MORE_DATA)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_NAME_ALREADY_EXISTS)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_NONEXCLUSIVE)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_NOT_FOUND)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_REMOTE_OUTOFMEMORY)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_SDK_COMPONENT_MISSING)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_SESSION_DISCONNECTED)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_UNSUPPORTED)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_WAIT_TIMEOUT)
            GR_D3D11_ERROR_CASE(DXGI_ERROR_WAS_STILL_DRAWING)
            GR_D3D11_ERROR_CASE(D3D10_ERROR_FILE_NOT_FOUND)
            GR_D3D11_ERROR_CASE(D3D10_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS)
            // GR_D3D11_ERROR_CASE(D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS)
            // GR_D3D11_ERROR_CASE(D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD)
            // GR_D3D11_ERROR_CASE(D3DERR_INVALIDCALL)
            // GR_D3D11_ERROR_CASE(D3DERR_WASSTILLDRAWING)
            default: return "-";
        }
    }

    void init_error(ID3D11Device* device)
    {
        g_device = device;
        ComPtr<ID3D11Debug> debug;
        if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug))) {

            ComPtr<ID3D11InfoQueue> info_queue;
            if (SUCCEEDED(debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&info_queue))) {
    #ifndef NDEBUG
                info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                info_queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
    #endif
            }
        }
    }

    void fatal_error(HRESULT hr, const char* fun)
    {
        const char* code_name = get_hresult_code_name(hr);
        std::string error_desc = get_win32_error_description(hr);
        xlog::error("%s failed: %lx (%s)\n%s", fun, hr, code_name, error_desc.c_str());
        if (hr == DXGI_ERROR_DEVICE_REMOVED) {
            hr = g_device->GetDeviceRemovedReason();
            code_name = get_hresult_code_name(hr);
            xlog::error("Device removed reason: %lx %s", hr, code_name);
        }
        RF_DEBUG_ERROR("D3D11 subsystem fatal error");
    }
}
