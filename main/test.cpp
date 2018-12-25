#include "stdafx.h"
#include "rf.h"
#include "utils.h"
#include "commands.h"

using namespace rf;

//#define EMULATE_PACKET_LOSS
#define PACKET_LOSS_RATE 2 // every n packet is lost

#ifdef EMULATE_PACKET_LOSS

static const auto NwSend = (int(*)(const void *pPacket, unsigned int cbPacket, int Flags, const NwAddr *pAddr, int MainType))0x00528820;
static const auto NwAddPacketToBuffer = (void(*)(void *pBuffer, const void *pData, unsigned int cbData, NwAddr *pAddr))0x00528950;

auto NwSend_Hook = makeFunHook(NwSend);
auto NwAddPacketToBuffer_Hook = makeFunHook(NwAddPacketToBuffer);

int NwSend_New(const void *pPacket, unsigned int cbPacket, int Flags, const NwAddr *pAddr, int MainType)
{
    if (rand() % PACKET_LOSS_RATE == 0)
        return 0;
    return NwSend_Hook.callTrampoline(pPacket, cbPacket, Flags, pAddr, MainType);
}

void NwAddPacketToBuffer_New(void *pBuffer, const void *pData, unsigned int cbData, NwAddr *pAddr)
{
    if (rand() % PACKET_LOSS_RATE == 0)
        return;
    return NwAddPacketToBuffer_Hook.callTrampoline(pBuffer, pData, cbData, pAddr);
}

#endif // EMULATE_PACKET_LOSS

#ifdef DEBUG

static void TestCmdHandler(void)
{
    if (rf::g_bDcRun)
    {
        //RfCmdGetNextArg(CMD_ARG_FLOAT, 0);
        //*((float*)0x017C7BD8) = *g_pfCmdArg;

        //int *ptr = (int*)HeapAlloc(GetProcessHeap(), 0, 8);
        //ptr[10] = 1;
        //DcPrintf("test done %p", ptr);
    }

    if (rf::g_bDcHelp)
        rf::DcPrintf("     test <n>", NULL);
}

static rf::DcCommand TestCmd = { "test", "Test command", TestCmdHandler };

void UiButton_BmGetBitmapSizeHook(int BmHandle, int *pWidth, int *pHeight)
{
    struct
    {
        const char *name;
        int w, h;
    } ButtonSize[] = {
        { "button.tga", 204, 61 },
        { "button_more.tga", 204, 61 },
        { "cyclebutton_l.tga", 19, 26 },
        { "cyclebutton_r.tga", 19, 26 },
        { "cyclebutton_u.tga", 21, 18 },
        { "cyclebutton_d.tga", 21, 18 },
        { "arrow_up_long.tga", 102, 20 },
        { "arrow_down_long.tga", 110, 20 },
        { "main_title.vbm", 512, 366 },
        { "indicator.tga", 14, 28 },
        { "sort_arrow.tga", 16, 16 },
    };

    BmGetBitmapSize(BmHandle, pWidth, pHeight);

    const char *Filename = BmGetFilename(BmHandle);
    for (unsigned i = 0; i < COUNTOF(ButtonSize); ++i)
    {
        if (!strcmp(Filename, ButtonSize[i].name))
        {
            *pWidth = ButtonSize[i].w;
            *pHeight = ButtonSize[i].h;
            return;
        }
    }

    ERR("Unknown UiButton texture: %s %d %d", Filename, *pWidth, *pHeight);
}

#include <MemUtils.h>

void TestInit()
{
#if 0
    WriteMemInt32(0x0051CB3B + 1, (uintptr_t)SortBonesByLevelHook - (0x0051CB3B + 0x5));
    //WriteMemUInt8(0x0051BA80, ASM_RET);

    WriteMemUInt8(0x00503360, ASM_LONG_JMP_REL);
    WriteMemInt32(0x00503360 + 1, (uintptr_t)AnimMeshUpdateTimeHook - (0x00503360 + 0x5));

    WriteMemInt32(0x0051BA1A + 1, (uintptr_t)CharacterAnimatorUpdateBonesHook - (0x0051BA1A + 0x5));

    WriteMemInt32(0x0051B81F + 1, (uintptr_t)CharacterAnimatorBuildTransformHook - (0x0051B81F + 0x5));
#endif
    CommandRegister(&TestCmd);

    // Change no-focus sleep argument
    WriteMemInt32(0x004353D3 + 1, 100);

    //WriteMemUInt32(0x00488BE9, ((uintptr_t)RenderEntityHook) - (0x00488BE8 + 0x5));

    // menu buttons speed
    //WriteMemFloat(0x00598FC0, 0.5f);
    //WriteMemFloat(0x00598FC4, -0.5f);
    
#if 0 // 32 bit texture formats
    // default: D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4, D3DFMT_A8R3G3B2
    // Note: RF code is broken for 32-bit formats. We can fix it to have better alpha textures (only 4 bits per channel is super small)
    // What will change - menu should look better except VBM files - they dont support 32-bit colors
    WriteMemUInt32(0x005A7DFC, D3DFMT_R8G8B8);
    WriteMemUInt32(0x005A7E00, D3DFMT_R5G6B5);
    WriteMemUInt32(0x005A7E04, D3DFMT_A8R8G8B8); // lightmaps
    WriteMemUInt32(0x005A7E08, D3DFMT_A8R8G8B8);
    WriteMemUInt32(0x005A7E0C, D3DFMT_A4R4G4B4);

    WriteMemInt32(0x0055B80E + 1, (uintptr_t)GrD3DSetTextureDataHook - (0x0055B80E + 0x5));
    WriteMemInt32(0x0055CA0B + 1, (uintptr_t)GrD3DSetTextureDataHook - (0x0055CA0B + 0x5));

    WriteMemUInt8(0x004ED3F5, ASM_PUSH_EBX);
    WriteMemInt32(0x004ED3F6 + 1, (uintptr_t)RflLoadLightmaps_004ED3F6 - (0x004ED3F6 + 0x5));
#endif

#if 0 // FIXME: rocket launcher screen is broken
    // Improved Railgun Scanner resolution
    const int8_t RailResolution = 120; // default is 64, max is 127 (signed byte)
    WriteMemUInt8(0x004AE0B7 + 1, RailResolution);
    WriteMemUInt8(0x004AE0B9 + 1, RailResolution);
    WriteMemUInt8(0x004ADD70 + 1, RailResolution);
    WriteMemUInt8(0x004ADD72 + 1, RailResolution);
    WriteMemUInt8(0x004A34BB + 1, RailResolution);
    WriteMemUInt8(0x004A34BD + 1, RailResolution);
    WriteMemUInt8(0x004325E6 + 1, RailResolution);
    WriteMemUInt8(0x004325E8 + 1, RailResolution);
#endif

#if 0 // tests for edge fixes
    // fog test
    WriteMemInt32(0x00431EA5 + 1, (uintptr_t)FogClearHook - (0x00431EA5 + 0x5));
    WriteMemUInt8(0x0043297A, ASM_NOP, 5);

    WriteMemInt32(0x00431D14 + 1, (uintptr_t)GrSetViewMatrixHook - (0x00431D14 + 0x5));
   
#endif

    WriteMemInt32(0x00457525 + 1, (uintptr_t)UiButton_BmGetBitmapSizeHook - (0x00457525 + 0x5));

#if 0
    // Bink
    WriteMemUInt8(0x00520AE0, ASM_NOP, 0x00520AED - 0x00520AE0);
    WriteMemUInt8(0x00520AF9, ASM_NOP, 0x00520B06 - 0x00520AF9);
#endif

#ifdef EMULATE_PACKET_LOSS
    NwSend_Hook.hook(NwSend_New);
    NwAddPacketToBuffer_Hook.hook(NwAddPacketToBuffer_New);
#endif
}

void TestRenderInGame()
{

}

void TestInitAfterGame()
{

}

void TestRender()
{
    char Buffer[256];
    int x = 50, y = 200;
    EntityObj *pEntity = g_pLocalPlayer ? rf::EntityGetFromHandle(g_pLocalPlayer->hEntity) : nullptr;
    rf::GrSetColor(255, 255, 255, 255);
    //MemReference<float, 0x005A4014> FrameTime;
    auto &FrameTime = *(float*)0x005A4014;

    if (pEntity)
    {
        sprintf(Buffer, "Flags 0x%X", pEntity->_Super.Flags);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        sprintf(Buffer, "PhysInfo Flags 0x%X", pEntity->_Super.PhysInfo.Flags);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        sprintf(Buffer, "MovementMode 0x%X", pEntity->MovementMode->Id);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        sprintf(Buffer, "pEntity 0x%p", pEntity);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        sprintf(Buffer, "framerate %f", FrameTime);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        static const auto *g_pAppTimeMs = (int*)0x5A3ED8;
        sprintf(Buffer, "AppTimeMs %d", *g_pAppTimeMs);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        sprintf(Buffer, "WeaponClsId %d", pEntity->WeaponInfo.WeaponClsId);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
        sprintf(Buffer, "Max Texture Size %lux%lu", rf::g_GrScreen.MaxTexWidthUnk3C, rf::g_GrScreen.MaxTexHeightUnk40);
        GrDrawText(x, y, Buffer, -1, rf::g_GrTextMaterial);
        y += 15;
    }
}

#endif // DEBUG
