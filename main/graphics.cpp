#include "graphics.h"
#include "commands.h"
#include "gr_color.h"
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include <CallHook.h>
#include <RegsPatch.h>
#include <ShortTypes.h>

namespace rf
{

static auto& g_Direct3D = AddrAsRef<IDirect3D8*>(0x01CFCBE0);
static auto& g_GrPP = AddrAsRef<D3DPRESENT_PARAMETERS>(0x01CFCA18);
static auto& g_AdapterIdx = AddrAsRef<uint32_t>(0x01CFCC34);
static auto& g_GrScaleVec = AddrAsRef<Vector3>(0x01818B48);
static auto& g_GrViewMatrix = AddrAsRef<Matrix3>(0x018186C8);
static auto& g_GrDeviceCaps = AddrAsRef<D3DCAPS8>(0x01CFCAC8);
static auto& g_GrDefaultWFar = AddrAsRef<float>(0x00596140);

static auto& GrSetTextureMipFilter = AddrAsRef<void(int linear)>(0x0050E830);
} // namespace rf

static float g_GrClippedGeomOffsetX = -0.5;
static float g_GrClippedGeomOffsetY = -0.5;

static void SetTextureMinMagFilterInCode(D3DTEXTUREFILTERTYPE filter_type)
{
    // clang-format off
    uintptr_t addresses[] = {
        // GrInitD3D
        0x00546283, 0x00546295,
        0x005462A9, 0x005462BD,
        // GrD3DSetMaterialFlags
        0x0054F2E8, 0x0054F2FC,
        0x0054F438, 0x0054F44C,
        0x0054F567, 0x0054F57B,
        0x0054F62D, 0x0054F641,
        0x0054F709, 0x0054F71D,
        0x0054F800, 0x0054F814,
        0x0054F909, 0x0054F91D,
        0x0054FA1A, 0x0054FA2E,
        0x0054FB22, 0x0054FB36,
        0x0054FBFE, 0x0054FC12,
        0x0054FD06, 0x0054FD1A,
        0x0054FD90, 0x0054FDA4,
        0x0054FE98, 0x0054FEAC,
        0x0054FF74, 0x0054FF88,
        0x0055007C, 0x00550090,
        0x005500C6, 0x005500DA,
        0x005501A2, 0x005501B6,
    };
    // clang-format on
    unsigned i;
    for (i = 0; i < std::size(addresses); ++i) WriteMem<u8>(addresses[i] + 1, (uint8_t)filter_type);
}

RegsPatch GrSetViewMatrix_widescreen_fix{
    0x005473AD,
    [](auto& regs) {
        (void)regs; // unused parameter

        constexpr float ref_aspect_ratio = 4.0f / 3.0f;
        constexpr float max_wide_aspect_ratio = 21.0f / 9.0f; // biggest aspect ratio currently in market

        // g_GrScreen.fAspect == ScrW / ScrH * 0.75 (1.0 for 4:3 monitors, 1.2 for 16:10) - looks like Pixel Aspect
        // Ratio We use here MaxWidth and MaxHeight to calculate proper FOV for windowed mode

        float viewport_aspect_ratio = static_cast<float>(rf::g_GrScreen.ViewportWidth) / rf::g_GrScreen.ViewportHeight;
        float aspect_ratio = static_cast<float>(rf::g_GrScreen.MaxWidth) / rf::g_GrScreen.MaxHeight;
        float scale_x = 1.0f;
        // this is how RF does compute scale_y and it is needed for working scanner
        float scale_y = ref_aspect_ratio * viewport_aspect_ratio / aspect_ratio;

        if (aspect_ratio <= max_wide_aspect_ratio) // never make X scale too high in windowed mode
            scale_x *= ref_aspect_ratio / aspect_ratio;
        else {
            scale_x *= ref_aspect_ratio / max_wide_aspect_ratio;
            scale_y *= aspect_ratio / max_wide_aspect_ratio;
        }

        rf::g_GrScaleVec.x *= scale_x; // Note: original division by aspect ratio is noped
        rf::g_GrScaleVec.y *= scale_y;

        g_GrClippedGeomOffsetX = rf::g_GrScreen.OffsetX - 0.5f;
        g_GrClippedGeomOffsetY = rf::g_GrScreen.OffsetY - 0.5f;
        static auto& gr_viewport_center_x = *(float*)0x01818B54;
        static auto& gr_viewport_center_y = *(float*)0x01818B5C;
        gr_viewport_center_x -= 0.5f; // viewport center x
        gr_viewport_center_y -= 0.5f; // viewport center y
    },
};

RegsPatch GrCreateD3DDevice_error_patch{
    0x00545CBD, [](auto& regs) {
        auto hr = static_cast<HRESULT>(regs.eax);
        ERR("D3D CreateDevice failed (hr 0x%X - %s)", hr, getDxErrorStr(hr));

        auto text = StringFormat("Failed to create Direct3D device object - error 0x%lX (%s).\n"
                                 "A critical error has occurred and the program cannot continue.\n"
                                 "Press OK to exit the program",
                                 hr, getDxErrorStr(hr));
        MessageBoxA(nullptr, text.c_str(), "Error!", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TASKMODAL);
        ExitProcess(-1);
    }};

RegsPatch GrClearZBuffer_fix_rect{0x00550A19, [](auto& regs) {
                                      auto& rect = *reinterpret_cast<D3DRECT*>(regs.edx);
                                      rect.x2++;
                                      rect.y2++;
                                  }};

static void SetupPP()
{
    memset(&rf::g_GrPP, 0, sizeof(rf::g_GrPP));

    static auto& format = *(D3DFORMAT*)0x005A135C;
    INFO("D3D Format: %ld", format);

    // Note: in MSAA mode we don't lock back buffer
#if !D3D_SWAP_DISCARD
    rf::g_GrPP.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
#endif

#if MULTISAMPLING_SUPPORT
    if (g_game_config.msaa && format > 0) {
        // Make sure selected MSAA mode is available
        HRESULT hr = rf::g_Direct3D->CheckDeviceMultiSampleType(rf::g_AdapterIdx, D3DDEVTYPE_HAL, format,
                                                                g_game_config.wndMode != GameConfig::FULLSCREEN,
                                                                (D3DMULTISAMPLE_TYPE)g_game_config.msaa);
        if (SUCCEEDED(hr)) {
            INFO("Enabling Anti-Aliasing (%ux MSAA)...", g_game_config.msaa);
            rf::g_GrPP.MultiSampleType = (D3DMULTISAMPLE_TYPE)g_game_config.msaa;
        }
        else {
            WARN("MSAA not supported (0x%x)...", hr);
            g_game_config.msaa = D3DMULTISAMPLE_NONE;
        }
    }
#endif

    // Make sure stretched window is always full screen
    if (g_game_config.wndMode == GameConfig::STRETCHED)
        SetWindowPos(rf::g_hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                     SWP_NOZORDER);
}

CallHook<void(int, rf::GrVertex**, int, int)> GrDrawRect_GrDrawPoly_Hook{
    0x0050DD69,
    [](int num, rf::GrVertex** pp_vertices, int flags, int mat) {
        for (int i = 0; i < num; ++i) {
            pp_vertices[i]->ScreenPos.x -= 0.5f;
            pp_vertices[i]->ScreenPos.y -= 0.5f;
        }
        GrDrawRect_GrDrawPoly_Hook.CallTarget(num, pp_vertices, flags, mat);
    },
};

DWORD SetupMaxAnisotropy()
{
    DWORD anisotropy_level = std::min(rf::g_GrDeviceCaps.MaxAnisotropy, 16ul);
    rf::g_GrDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, anisotropy_level);
    rf::g_GrDevice->SetTextureStageState(1, D3DTSS_MAXANISOTROPY, anisotropy_level);
    return anisotropy_level;
}

CallHook<void()> GrInitBuffers_AfterReset_Hook{
    0x00545045,
    []() {
        GrInitBuffers_AfterReset_Hook.CallTarget();

        // Apply state change after reset
        // Note: we dont have to set min/mag filtering because its set when selecting material

        rf::g_GrDevice->SetRenderState(D3DRS_CULLMODE, 1);
        rf::g_GrDevice->SetRenderState(D3DRS_SHADEMODE, 2);
        rf::g_GrDevice->SetRenderState(D3DRS_SPECULARENABLE, 0);
        rf::g_GrDevice->SetRenderState(D3DRS_AMBIENT, 0xFF545454);
        rf::g_GrDevice->SetRenderState(D3DRS_CLIPPING, 0);

        if (rf::g_LocalPlayer)
            rf::GrSetTextureMipFilter(rf::g_LocalPlayer->Config.FilteringLevel == 0);

        if (rf::g_GrDeviceCaps.MaxAnisotropy > 0 && g_game_config.anisotropicFiltering)
            SetupMaxAnisotropy();
    },
};

bool g_reset_device_req = false;

RegsPatch switch_d3d_mode_patch{
    0x0054500D,
    [](auto& regs) {
        if (g_reset_device_req) {
            g_reset_device_req = false;
            regs.eip = 0x00545017;
        }
    },
};

DcCommand2 fullscreen_cmd{
    "fullscreen",
    []() {
        rf::g_GrPP.Windowed = false;
        g_reset_device_req = true;
    },
};

DcCommand2 windowed_cmd{
    "windowed",
    []() {
        rf::g_GrPP.Windowed = true;
        g_reset_device_req = true;
    },
};

void GraphicsInit()
{
    // Fix for "At least 8 MB of available video memory"
    WriteMem<u8>(0x005460CD, ASM_JAE_SHORT);

#if WINDOWED_MODE_SUPPORT
    if (g_game_config.wndMode != GameConfig::FULLSCREEN) {
        /* Enable windowed mode */
        WriteMem<u32>(0x004B29A5 + 6, 0xC8);
        if (g_game_config.wndMode == GameConfig::STRETCHED) {
            uint32_t wnd_style = WS_POPUP | WS_SYSMENU;
            WriteMem<u32>(0x0050C474 + 1, wnd_style);
            WriteMem<u32>(0x0050C4E3 + 1, wnd_style);
        }
    }
#endif

    // WriteMem<u8>(0x00524C98, ASM_SHORT_JMP_REL); // disable window hooks

#if D3D_SWAP_DISCARD
    // Use Discard Swap Mode
    WriteMem<u32>(0x00545B4D + 6, D3DSWAPEFFECT_DISCARD); // full screen
    WriteMem<u32>(0x00545AE3 + 6, D3DSWAPEFFECT_DISCARD); // windowed
#endif

    // Replace memset call to SetupPP
    AsmWritter(0x00545AA7, 0x00545AB5).nop();
    AsmWritter(0x00545AA7).call(SetupPP);
    AsmWritter(0x00545BA5).nop(6); // dont set PP.Flags

    // nVidia issue fix (make sure D3Dsc is enabled)
    WriteMem<u8>(0x00546154, 1);

    // Properly restore state after device reset
    GrInitBuffers_AfterReset_Hook.Install();

#if WIDESCREEN_FIX
    // Fix FOV for widescreen
    AsmWritter(0x00547354, 0x00547358).nop();
    GrSetViewMatrix_widescreen_fix.Install();
    WriteMem<float>(0x0058A29C, 0.0003f); // factor related to near plane, default is 0.000588f
#endif

    // Don't use LOD models
    if (g_game_config.disableLodModels) {
        // WriteMem<u8>(0x00421A40, ASM_SHORT_JMP_REL);
        WriteMem<u8>(0x0052FACC, ASM_SHORT_JMP_REL);
    }

    // Better error message in case of device creation error
    GrCreateD3DDevice_error_patch.Install();

    // Optimization - remove unused back buffer locking/unlocking in GrSwapBuffers
    AsmWritter(0x0054504A).jmp(0x0054508B);

#if 1
    // Fix rendering of right and bottom edges of viewport
    WriteMem<u8>(0x00431D9F, ASM_SHORT_JMP_REL);
    WriteMem<u8>(0x00431F6B, ASM_SHORT_JMP_REL);
    WriteMem<u8>(0x004328CF, ASM_SHORT_JMP_REL);
    AsmWritter(0x0043298F).jmp(0x004329DC);
    GrClearZBuffer_fix_rect.Install();

    // Left and top viewport edge fix for MSAA (RF does similar thing in GrDrawTextureD3D)
    WriteMem<u8>(0x005478C6, ASM_FADD);
    WriteMem<u8>(0x005478D7, ASM_FADD);
    WriteMemPtr(0x005478C6 + 2, &g_GrClippedGeomOffsetX);
    WriteMemPtr(0x005478D7 + 2, &g_GrClippedGeomOffsetY);
    GrDrawRect_GrDrawPoly_Hook.Install();
#endif

    if (g_game_config.highScannerRes) {
        // Improved Railgun Scanner resolution
        constexpr int8_t scanner_resolution = 120;        // default is 64, max is 127 (signed byte)
        WriteMem<u8>(0x004325E6 + 1, scanner_resolution); // RenderInGame
        WriteMem<u8>(0x004325E8 + 1, scanner_resolution);
        WriteMem<u8>(0x004A34BB + 1, scanner_resolution); // PlayerCreate
        WriteMem<u8>(0x004A34BD + 1, scanner_resolution);
        WriteMem<u8>(0x004ADD70 + 1, scanner_resolution); // PlayerRenderRailgunScannerViewToTexture
        WriteMem<u8>(0x004ADD72 + 1, scanner_resolution);
        WriteMem<u8>(0x004AE0B7 + 1, scanner_resolution);
        WriteMem<u8>(0x004AE0B9 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF0B0 + 1, scanner_resolution); // PlayerRenderScannerView
        WriteMem<u8>(0x004AF0B4 + 1, scanner_resolution * 3 / 4);
        WriteMem<u8>(0x004AF0B6 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF7B0 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF7B2 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF7CF + 1, scanner_resolution);
        WriteMem<u8>(0x004AF7D1 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF818 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF81A + 1, scanner_resolution);
        WriteMem<u8>(0x004AF820 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF822 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF855 + 1, scanner_resolution);
        WriteMem<u8>(0x004AF860 + 1, scanner_resolution * 3 / 4);
        WriteMem<u8>(0x004AF862 + 1, scanner_resolution);
    }

    // Always transfer entire framebuffer to entire window in Present call
    WriteMem<u8>(0x00544FE6, ASM_SHORT_JMP_REL);

    // Init True Color improvements
    GrColorInit();

    // Enable mip-mapping for textures bigger than 256x256
    AsmWritter(0x0050FEDA, 0x0050FEE9).nop();
    WriteMem<u8>(0x0055B739, ASM_SHORT_JMP_REL);

    // Fix decal fade out
    WriteMem<u8>(0x00560994 + 1, 3);

    // fullscreen/windowed commands
    switch_d3d_mode_patch.Install();
    fullscreen_cmd.Register();
    windowed_cmd.Register();
}

void GraphicsAfterGameInit()
{
#if ANISOTROPIC_FILTERING
    // Anisotropic texture filtering
    if (rf::g_GrDeviceCaps.MaxAnisotropy > 0 && g_game_config.anisotropicFiltering && !rf::g_IsDedicatedServer) {
        SetTextureMinMagFilterInCode(D3DTEXF_ANISOTROPIC);
        DWORD anisotropy_level = SetupMaxAnisotropy();
        INFO("Anisotropic Filtering enabled (level: %d)", anisotropy_level);
    }
#endif

    // Change font for Time Left text
    static int time_left_font = rf::GrLoadFont("rfpc-large.vf", -1);
    if (time_left_font >= 0) {
        WriteMem<i8>(0x00477157 + 1, time_left_font);
        WriteMem<i8>(0x0047715F + 2, 21);
        WriteMem<i32>(0x00477168 + 1, 154);
    }
}

void GraphicsDrawFpsCounter()
{
    if (g_game_config.fpsCounter) {
        auto text = StringFormat("FPS: %.1f", rf::g_fFps);
        rf::GrSetColor(0, 255, 0, 255);
        rf::GrDrawAlignedText(rf::GR_ALIGN_RIGHT, rf::GrGetMaxWidth() - 10, 60, text.c_str(), -1, rf::g_GrTextMaterial);
    }
}
