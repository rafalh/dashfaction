#pragma once

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef BOOL WINBOOL;

#include <d3d8.h>

#ifndef __GNUC__
#define ALIGN(n) __declspec(align(n))
#else
#define ALIGN(n) __attribute__((aligned(n)))
#endif

#pragma pack(push, 1)

namespace rf
{
    /* Declarations */
    struct CPlayer;
    struct EntityObj;
    struct CAnimMesh;

    /* Math */

    struct CVector3
    {
        float x, y, z;

        CVector3() {}

        CVector3(float x, float y, float z) :
            x(x), y(y), z(z) {}

        CVector3 &operator+=(const CVector3 &other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        CVector3 &operator*=(float m)
        {
            x *= m;
            y *= m;
            z *= m;
            return *this;
        }

        CVector3 operator-() const
        {
            return CVector3(-x, -y, -z);
        }

        CVector3 &operator-=(const CVector3 &other)
        {
            return (*this += -other);
        }

        CVector3 operator+(const CVector3 &other) const
        {
            CVector3 tmp = *this;
            tmp += other;
            return tmp;
        }

        CVector3 operator-(const CVector3 &other) const
        {
            CVector3 tmp = *this;
            tmp -= other;
            return tmp;
        }

        CVector3 operator*(float m) const
        {
            return CVector3(x * m, y * m, z * m);
        }

        float len() const
        {
            return sqrtf(lenPow2());
        }

        float lenPow2() const
        {
            return x * x + y * y + z * z;
        }
    };

    struct CMatrix3
    {
        CVector3 rows[3];
    };

    /* String */

    struct CString
    {
        uint32_t cch;
        char *psz;
    };

    static const auto StringAlloc = (char*(*)(unsigned cbSize))0x004FF300;
    static const auto StringFree = (void(*)(void *pData))0x004FF3A0;
    static const auto CString_Init = (CString*(__thiscall *)(CString *This, const char *pszInit))0x004FF3D0;
    static const auto CString_InitFromStr = (CString*(__thiscall *)(CString *This, const CString *pstrInit))0x004FF410;
    static const auto CString_InitEmpty = (CString*(__thiscall *)(CString *This))0x004FF3B0;
    static const auto CString_CStr = (const char*(__thiscall *)(CString *This))0x004FF480;
    static const auto CString_Destroy = (void(__thiscall *)(CString *This))0x004FF470;
    static const auto CString_Assign = (CString*(__thiscall *)(CString *This, CString *Src))0x004FFA20;

    /* Utils */

    struct Timer
    {
        int EndTimeMs;
    };

    struct Color
    {
        int Val;
    };

    struct DynamicArray
    {
        int Size;
        int AllocatedSize;
        void *pData;
    };

    /* Stubs */
    typedef int ObjectFlags;
    typedef int PhysicsFlags;
    typedef int EntityFlags;
    typedef int EntityPowerups;
    typedef int PlayerFlags;
    typedef int NwPlayerFlags;

    /* Debug Console */

    struct DcCommand
    {
        const char *pszCmd;
        const char *pszDescr;
        void(*pfnHandler)(void);
    };

    enum DcArgType
    {
        DC_ARG_NONE = 0x0001,
        DC_ARG_STR = 0x0002,
        DC_ARG_INT = 0x0008,
        DC_ARG_FLOAT = 0x0010,
        DC_ARG_HEX = 0x0020,
        DC_ARG_TRUE = 0x0040,
        DC_ARG_FALSE = 0x0080,
        DC_ARG_PLUS = 0x0100,
        DC_ARG_MINUS = 0x0200,
        DC_ARG_COMMA = 0x0400,
    };

    static const auto DcPrint = (void(*)(const char *pszText, const int *pColor))0x00509EC0;
    static const auto DcPrintf = (void(*)(const char *pszFormat, ...))0x0050B9F0;
    static const auto DcGetArg = (void(*)(int Type, int bUnknown))0x0050AED0;
    static const auto DcAutoCompleteInput = (void(*)())0x0050A620;
    static const auto DcRunCmd = (int(*)(const char *pszCmd))0x00509B00;

    typedef void(*DcCmdHandler)(void);
    static const auto DcCommand__Init = (void(__thiscall *)(rf::DcCommand *This, const char *pszCmd, const char *pszDescr, DcCmdHandler pfnHandler))0x00509A70;

#define DC_REGISTER_CMD(name, help, handler) \
    do { \
        static rf::DcCommand Dc_##name; \
        rf::DcCommand__Init(&Dc_##name, #name, help, handler); \
    } while (false)

    //static const auto g_ppDcCommands = (DcCommand**)0x01775530;
    static auto &g_DcNumCommands = *(uint32_t*)0x0177567C;

    static auto &g_bDcRun = *(uint32_t*)0x01775110;
    static auto &g_bDcHelp = *(uint32_t*)0x01775224;
    static auto &g_bDcStatus = *(uint32_t*)0x01774D00;
    static auto &g_DcArgType = *(uint32_t*)0x01774D04;
    static const auto g_pszDcArg = (char*)0x0175462C;
    static auto &g_iDcArg = *(int*)0x01775220;
    static auto &g_fDcArg = *(float*)0x01754628;
    static const auto g_szDcCmdLine = (char*)0x01775330;
    static auto &g_cchDcCmdLineLen = *(uint32_t*)0x0177568C;

    /* Debug Console Commands */

    // Server configuration commands
    static const auto DcfKillLimit = (DcCmdHandler)0x0046CBC0;
    static const auto DcfTimeLimit = (DcCmdHandler)0x0046CC10;
    static const auto DcfGeomodLimit = (DcCmdHandler)0x0046CC70;
    static const auto DcfCaptureLimit = (DcCmdHandler)0x0046CCC0;

    // Misc commands
    static const auto DcfSound = (DcCmdHandler)0x00434590;
    static const auto DcfDifficulty = (DcCmdHandler)0x00434EB0;
    static const auto DcfMouseSensitivity = (DcCmdHandler)0x0043CE90;
    static const auto DcfLevelInfo = (DcCmdHandler)0x0045C210;
    static const auto DcfVerifyLevel = (DcCmdHandler)0x0045E1F0;
    static const auto DcfPlayerNames = (DcCmdHandler)0x0046CB80;
    static const auto DcfClientsCount = (DcCmdHandler)0x0046CD10;
    static const auto DcfKickAll = (DcCmdHandler)0x0047B9E0;
    static const auto DcfTimedemo = (DcCmdHandler)0x004CC1B0;
    static const auto DcfFramerateTest = (DcCmdHandler)0x004CC360;
    static const auto DcfSystemInfo = (DcCmdHandler)0x00525A60;
    static const auto DcfTrilinearFiltering = (DcCmdHandler)0x0054F050;
    static const auto DcfDetailTextures = (DcCmdHandler)0x0054F0B0;

    /* Graphics */

    struct GrScreen
    {
        int Signature;
        int MaxWidth;
        int MaxHeight;
        int Mode;
        int WindowMode;
        int field_14;
        float fAspect;
        int field_1C;
        int BitsPerPixel;
        int BytesBerPixel;
        int field_28;
        int OffsetX;
        int OffsetY;
        int ViewportWidth;
        int ViewportHeight;
        int MaxTexWidthUnk3C;
        int MaxTexHeightUnk40;
        int ClipLeft;
        int ClipRight;
        int ClipTop;
        int ClipBottom;
        int CurrentColor;
        int CurrentBitmap;
        int field_5C;
        int FogMode;
        int FogColor;
        float FogNear;
        float FogFar;
        float field_70;
        int field_74;
        float field_78;
        float field_7C;
        float field_80;
        int field_84;
        int field_88;
        int field_8C;
    };

    struct GrVertex
    {
        CVector3 v3dPos;
        CVector3 vScreenPos;
        int ClipFlags;
        float u0;
        float v0;
        float u1;
        float v1;
        int Color;
    };

    struct SGrLockData
    {
        int BmHandle;
        int SectionIdx;
        int PixelFormat;
        BYTE *pBits;
        int Width;
        int Height;
        int Pitch;
        int field_1C;
    };

    enum GrTextAlignment
    {
        GR_ALIGN_LEFT = 0,
        GR_ALIGN_CENTER = 1,
        GR_ALIGN_RIGHT = 2,
    };

    static auto &g_pDirect3D = *(IDirect3D8**)0x01CFCBE0;
    static auto &g_pGrDevice = *(IDirect3DDevice8**)0x01CFCBE4;
    static auto &g_GrScreen = *(GrScreen*)0x017C7BC0;
    static auto &g_GrPP = *(D3DPRESENT_PARAMETERS*)0x01CFCA18;
    static const auto g_pGrGammaRamp = (uint32_t*)0x017C7C68;
    static auto &g_AdapterIdx = *(uint32_t*)0x01CFCC34;
    static auto &g_GrScaleVec = *(CVector3*)0x01818B48;
    static auto &g_GrViewMatrix = *(CMatrix3*)0x018186C8;
    static auto &g_GrDeviceCaps = *(D3DCAPS8*)0x01CFCAC8;
    static auto &g_GrDefaultWFar = *(float*)0x00596140;

    static auto &g_GrLineMaterial = *(uint32_t*)0x01775B00;
    static auto &g_GrRectMaterial = *(uint32_t*)0x17756C0;
    static auto &g_GrTextMaterial = *(uint32_t*)0x17C7C5C;
    static auto &g_GrBitmapMaterial = *(uint32_t*)0x017756BC;
    static auto &g_GrImageMaterial = *(uint32_t*)0x017756DC;

    static const auto GrGetMaxWidth = (unsigned(*)())0x0050C640;
    static const auto GrGetMaxHeight = (unsigned(*)())0x0050C650;
    static const auto GrGetViewportWidth = (unsigned(*)())0x0050CDB0;
    static const auto GrGetViewportHeight = (unsigned(*)())0x0050CDC0;
    static const auto GrSetColor = (void(*)(unsigned r, unsigned g, unsigned b, unsigned a))0x0050CF80;
    static const auto GrSetColorPtr = (void(*)(uint32_t *pColor))0x0050D000;
    static const auto GrDrawRect = (void(*)(unsigned x, unsigned y, unsigned cx, unsigned cy, unsigned Material))0x0050DBE0;
    static const auto GrDrawImage = (void(*)(int BmHandle, int x, int y, int Material))0x0050D2A0;
    static const auto GrDrawBitmapStretched = (void(*)(int BmHandle, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, float a10, float a11, int Material))0x0050D250;
    static const auto GrDrawPoly = (void(*)(int Num, GrVertex **ppVertices, int Flags, int iMat))0x0050DF80;
    static const auto GrDrawText = (void(*)(unsigned x, unsigned y, const char *pszText, int Font, unsigned Material))0x0051FEB0;
    static const auto GrDrawAlignedText = (void(*)(GrTextAlignment Align, unsigned x, unsigned y, const char *pszText, int Font, unsigned Material))0x0051FE50;
    static const auto GrFitText = (CString *(*)(CString *pstrDest, CString Str, int cxMax))0x00471EC0;
    static const auto GrReadBackBuffer = (int(*)(int x, int y, int Width, int Height, void *pBuffer))0x0050DFF0;
    static const auto GrLoadFont = (int(*)(const char *pszFileName, int a2))0x0051F6E0;
    static const auto GrGetFontHeight = (unsigned(*)(int FontId))0x0051F4D0;
    static const auto GrGetTextWidth = (void(*)(int *pOutWidth, int *pOutHeight, const char *pszText, int TextLen, int FontId))0x0051F530;
    static const auto GrFlushBuffers = (void(*)())0x00559D90;
    static const auto GrSwapBuffers = (void(*)())0x0050CE20;
    static const auto GrSetViewMatrix = (void(*)(CMatrix3 *pMatRot, CVector3 *pPos, float fFov, int a4, int a5))0x00517EB0;
    static const auto GrSetViewMatrixD3D = (void(*)(CMatrix3 *pMatRot, CVector3 *pPos, float fFov, int a4, int a5))0x00547150;
    static const auto GrRenderLine = (char(*)(const CVector3 *pWorldPos1, const CVector3 *pWorldPos2, int Material))0x00515960;
    static const auto GrRenderSphere = (char(*)(const CVector3 *pvPos, float fRadius, int Material))0x00515CD0;
    static const auto GrLock = (char(*)(int BmHandle, int SectionIdx, SGrLockData *pData, int a4))0x0050E2E0;
    static const auto GrUnlock = (void(*)(SGrLockData *pData))0x0050E310;
    static const auto GrD3DSetTextureData = (int(*)(int Level, const BYTE *pSrcBits, const BYTE *pPallete, int cxBm, int cyBm, int PixelFmt, void *a7, int cxTex, int cyTex, IDirect3DTexture8 *pTextures))0x0055BA10;
    static const auto GrInitBuffers = (void(*)())0x005450A0;
    static const auto GrSetTextureMipFilter = (void(*)(int bLinear))0x0050E830;
    static const auto GrResetClip = (void(*)())0x0050CDD0;

    /* Bmpman */

    enum BmPixelFormat
    {
        BMPF_INVALID = 0x0,
        BMPF_MONO8 = 0x1,
        BMPF_UNK_2_8B = 0x2,
        BMPF_565 = 0x3,
        BMPF_4444 = 0x4,
        BMPF_1555 = 0x5,
        BMPF_888 = 0x6,
        BMPF_8888 = 0x7,
        BMPF_UNK_8_16B = 0x8,
    };

    static const auto BmLoad = (int(*)(const char *pszFilename, int a2, BOOL a3))0x0050F6A0;
    static const auto BmCreateUserBmap = (int(*)(BmPixelFormat PixelFormat, int Width, int Height))0x005119C0;
    static const auto BmConvertFormat = (void(*)(void *pDstBits, BmPixelFormat DstPixelFmt, const void *pSrcBits, BmPixelFormat SrcPixelFmt, int NumPixels))0x0055DD20;
    static const auto BmGetBitmapSize = (void(*)(int BmHandle, int *pWidth, int *pHeight))0x00510630;
    static const auto BmGetFilename = (const char*(*)(int BmHandle))0x00511710;
    static const auto BmLock = (BmPixelFormat(*)(int BmHandle, BYTE **ppData, BYTE **ppPalette))0x00510780;
    static const auto BmUnlock = (void(*)(int BmHandle))0x00511700;

    /* User Interface (UI) */

    struct UiPanel
    {
        void(__cdecl **field_0)();
        UiPanel *pParent;
        char field_8;
        char field_9;
        int x;
        int y;
        int w;
        int h;
        int Id;
        void(*OnClick)(void);
        int field_24;
        int BgTexture;
    };

    static const auto UiMsgBox = (void(*)(const char *pszTitle, const char *pszText, void(*pfnCallback)(void), BOOL bInput))0x004560B0;
    static const auto UiCreateDialog = (void(*)(const char *pszTitle, const char *pszText, unsigned cButtons, const char **ppszBtnTitles, void **ppfnCallbacks, unsigned Unknown1, unsigned Unknown2))0x004562A0;
    static const auto UiGetElementFromPos = (int(*)(int x, int y, UiPanel **ppGuiList, signed int cGuiList))0x00442ED0;
    static const auto UiLabel_Create2 = (void(__thiscall *)(UiPanel *pThis, UiPanel *pParent, int x, int y, int w, int h, const char *pszText, int FontId))0x00456C20;


    /* HUD */

#define HUD_POINTS_COUNT 48
    static const auto g_pHudPosData640 = (POINT*)0x00637868;
    static const auto g_pHudPosData800 = (POINT*)0x006373D0;
    static const auto g_pHudPosData1024 = (POINT*)0x00637230;
    static const auto g_pHudPosData1280 = (POINT*)0x00637560;
    static const auto g_pHudPosData = (POINT*)0x006376E8;

    typedef void(*ChatPrint_Type)(CString strText, unsigned ColorId, CString Prefix);
    static const auto ChatPrint = (ChatPrint_Type)0x004785A0;

    /* File System */

    struct PackfileEntry;

    struct Packfile
    {
        char szName[32];
        char szPath[128];
        uint32_t field_A0;
        uint32_t cFiles;
        PackfileEntry *pFileList;
        uint32_t cbSize;
    };

    struct PackfileEntry
    {
        uint32_t dwNameChecksum;
        const char *pszFileName;
        uint32_t OffsetInBlocks;
        uint32_t cbFileSize;
        Packfile *pArchive;
        FILE *pRawFile;
    };

    struct PackfileLookupTable
    {
        uint32_t dwNameChecksum;
        PackfileEntry *pArchiveEntry;
    };

    static auto &g_cArchives = *(uint32_t*)0x01BDB214;
    static auto &g_Archives = *(Packfile*)0x01BA7AC8;
#define VFS_LOOKUP_TABLE_SIZE 20713
    static auto &g_VfsLookupTable = *(PackfileLookupTable*)0x01BB2AC8;
    static auto &g_bVfsIgnoreTblFiles = *(uint8_t*)0x01BDB21C;

    typedef BOOL(*PackfileLoad_Type)(const char *pszFileName, const char *pszDir);
    static const auto PackfileLoad = (PackfileLoad_Type)0x0052C070;

    typedef Packfile *(*PackfileFindArchive_Type)(const char *pszFilename);
    static const auto PackfileFindArchive = (PackfileFindArchive_Type)0x0052C1D0;

    typedef uint32_t(*PackfileCalcFileNameChecksum_Type)(const char *pszFileName);
    static const auto PackfileCalcFileNameChecksum = (PackfileCalcFileNameChecksum_Type)0x0052BE70;

    typedef uint32_t(*PackfileAddToLookupTable_Type)(PackfileEntry *pPackfileEntry);
    static const auto PackfileAddToLookupTable = (PackfileAddToLookupTable_Type)0x0052BCA0;

    typedef uint32_t(*PackfileProcessHeader_Type)(Packfile *pPackfile, const void *pHeader);
    static const auto PackfileProcessHeader = (PackfileProcessHeader_Type)0x0052BD10;

    typedef uint32_t(*PackfileAddEntries_Type)(Packfile *pPackfile, const void *pBuf, unsigned cFilesInBlock, unsigned *pcAddedEntries);
    static const auto PackfileAddEntries = (PackfileAddEntries_Type)0x0052BD40;

    typedef uint32_t(*PackfileSetupFileOffsets_Type)(Packfile *pPackfile, unsigned DataOffsetInBlocks);
    static const auto PackfileSetupFileOffsets = (PackfileSetupFileOffsets_Type)0x0052BEB0;

    static const auto FsAddDirectoryEx = (int(*)(const char *pszDir, const char *pszExtList, char bUnknown))0x00514070;

    static const auto FileGetChecksum = (unsigned(*)(const char *pszFilename))0x00436630;

    /* Network */

    struct NwAddr
    {
        uint32_t IpAddr;
        uint16_t Port;
        uint16_t _unused;
    };

    struct NwStats
    {
        int NumTotalSentBytes;
        int NumTotalRecvBytes;
        int NumSentBytesUnkIdxF8[30];
        int NumRecvBytesUnkIdxF8[30];
        int UnkIdxF8;
        int NumSentBytesUnkIdx1EC[30];
        int NumRecvBytesUnkIdx1EC[30];
        int UnkIdx1EC;
        Timer field_1F0;
        int NumObjUpdatePacketsSent;
        int NumObjUpdatePacketsRecv;
        int field_1FC[3];
        int NumSentBytesPerPacket[55];
        int NumRecvBytesPerPacket[55];
        int NumPacketsSent[55];
        int NumPacketsRecv[55];
        Timer field_578;
        int field_57C;
        int field_580;
        int field_584;
        Timer field_588;
        int field_58C;
    };

    struct CPlayerNetData
    {
        NwAddr Addr;
        NwPlayerFlags Flags;
        int field_C;
        int ConnectionId;
        uint8_t PlayerId;
        uint8_t _unused[3];
        int field_18;
        NwStats Stats;
        int dwPing;
        float field_5B0;
        char PacketBuf[512];
        int cbPacketBuf;
        char OutReliableBuf[512];
        int cbOutReliableBuf;
        int ConnectionSpeed;
        int field_9C0;
        Timer field_9C4;
    };
    static_assert(sizeof(CPlayerNetData) == 0x9C8, "invalid size");

    static auto &g_NwSocket = *(SOCKET*)0x005A660C;

    typedef void(*NwProcessGamePackets_Type)(const char *pData, int cbData, const NwAddr *pAddr, CPlayer *pPlayer);
    static const auto NwProcessGamePackets = (NwProcessGamePackets_Type)0x004790D0;

    typedef void(*NwSendNotReliablePacket_Type)(const void *pAddr, const void *pPacket, unsigned cbPacket);
    static const auto NwSendNotReliablePacket = (NwSendNotReliablePacket_Type)0x0052A080;

    static const auto NwSendReliablePacket = (void(*)(CPlayer *pPlayer, const BYTE *pData, unsigned int cbData, int a4))0x00479480;

    typedef void(*NwAddrToStr_Type)(char *pszDest, int cbDest, NwAddr *pAddr);
    static const auto NwAddrToStr = (NwAddrToStr_Type)0x00529FE0;

    static const auto NwGetPlayerFromAddr = (CPlayer *(*)(const NwAddr *pAddr))0x00484850;
    static const auto NwCompareAddr = (int(*)(const NwAddr *pAddr1, const NwAddr *pAddr2, char bCheckPort))0x0052A930;

    /* Camera */

    enum CameraType
    {
        CAM_FIRST_PERSON = 0x0,
        CAM_THIRD_PERSON = 0x1,
        CAM_FREELOOK = 0x2,
        CAM_3 = 0x3,
        CAM_FREELOOK2 = 0x4,
        CAM_CUTSCENE_UNK = 0x5,
        CAM_FIXED = 0x6,
        CAM_ORBIT = 0x7,
    };


    struct Camera
    {
        EntityObj *pCameraEntity;
        CPlayer *pPlayer;
        CameraType Type;
    };

    static const auto CameraSetFirstPerson = (void(*)(Camera *pCamera))0x0040DDF0;
    static const auto CameraSetFreelook = (void(*)(Camera *pCamera))0x0040DCF0;

    /* Config */

    struct ControlConfigItem
    {
        __int16 DefaultScanCodes[2];
        __int16 DefaultMouseBtnId;
        __int16 field_6;
        int field_8;
        CString strName;
        __int16 ScanCodes[2];
        __int16 MouseBtnId;
        __int16 field_1A;
    };

    struct ControlConfig
    {
        float fMouseSensitivity;
        int bMouseLook;
        int field_EC;
        ControlConfigItem Keys[128];
        int CtrlCount;
        int field_EF4;
        int field_EF8;
        int field_EFC;
        int field_F00;
        char field_F04;
        char bMouseYInvert;
        char field_E22;
        char field_E23;
        int field_F08;
        int field_F0C;
        int field_F10;
        int field_F14;
        int field_F18;
        int field_F1C;
        int field_F20;
        int field_F24;
        int field_F28;
    };

    struct PlayerConfig
    {
        ControlConfig Controls;
        int field_F2C;
        int field_F30;
        char field_F34;
        char field_E51;
        char bFirstPersonWeaponVisible;
        char field_E53;
        char bCrouchStay;
        char field_F39;
        char field_F3A;
        char bHudVisible;
        char field_F3C;
        char field_E59;
        char field_E5A;
        char field_E5B;
        char bAutoswitchWeapons;
        char bAutoswitchExplosives;
        char ShadowsEnabled;
        char DecalsEnabled;
        char DynamicLightiningEnabled;
        char field_F45;
        char field_F46;
        char field_F47;
        char FilteringLevel;
        char field_F49;
        char field_F4A;
        char field_F4B;
        int DetailLevel;
        int TexturesResolutionLevel;
        int CharacterDetailLevel;
        float field_F58;
        int MpCharacter;
        char szName[12];
    };
    static_assert(sizeof(PlayerConfig) == 0xE88, "invalid size");

    enum EGameCtrl
    {
        GC_PRIMARY_ATTACK = 0x0,
        GC_SECONDARY_ATTACK = 0x1,
        GC_USE = 0x2,
        GC_JUMP = 0x3,
        GC_CROUCH = 0x4,
        GC_HIDE_WEAPON = 0x5,
        GC_RELOAD = 0x6,
        GC_NEXT_WEAPON = 0x7,
        GC_PREV_WEAPON = 0x8,
        GC_CHAT = 0x9,
        GC_TEAM_CHAT = 0xA,
        GC_FORWARD = 0xB,
        GC_BACKWARD = 0xC,
        GC_SLIDE_LEFT = 0xD,
        GC_SLIDE_RIGHT = 0xE,
        GC_SLIDE_UP = 0xF,
        GC_SLIDE_DOWN = 0x10,
        GC_LOOK_DOWN = 0x11,
        GC_LOOK_UP = 0x12,
        GC_TURN_LEFT = 0x13,
        GC_TURN_RIGHT = 0x14,
        GC_MESSAGES = 0x15,
        GC_MP_STATS = 0x16,
        GC_QUICK_SAVE = 0x17,
        GC_QUICK_LOAD = 0x18,
    };

    /* Player */

    struct SPlayerStats
    {
        uint16_t field_0;
        int16_t iScore;
        int16_t cCaps;
    };

    struct PlayerWeaponInfo
    {
        int NextWeaponClsId;
        Timer WeaponSwitchTimer;
        Timer UnkReloadTimer;
        int field_F8C;
        Timer UnkTimerF90;
        char bInScopeView;
        char field_F95;
        char field_F96;
        char field_F97;
        float fScopeZoom;
        char field_F9C;
        char field_1D;
        char field_1E;
        char field_1F;
        int field_FA0;
        Timer field_FA4;
        Timer TimerFA8;
        Timer field_FAC;
        char bRailgunScanner;
        char bScannerView;
        Color clrUnkR;
        char field_FB6;
        char field_FB7;
        int field_FB8;
        int field_FBC;
        float field_FC0;
        CVector3 field_FC4;
        CMatrix3 field_FD0;
        CMatrix3 matRotFpsWeapon;
        CVector3 vPosFpsWeapon;
        int field_1024;
        float field_1028;
        int field_102C;
        float field_1030;
        int Pivot1PropId;
        Timer field_1038;
        int field_103C;
        int field_1040;
        int RemoteChargeVisible;
        CVector3 field_1048;
        CMatrix3 field_1054;
        int field_1078;
        Timer field_107C;
    };
    static_assert(sizeof(PlayerWeaponInfo) == 0x100, "invalid size");

    struct Player_1094
    {
        CVector3 field_1094;
        CMatrix3 field_10A0;
    };

    struct CPlayer
    {
        CPlayer *pNext;
        CPlayer *pPrev;
        CString strName;
        PlayerFlags Flags;
        int hEntity;
        int EntityClsId;
        CVector3 field_1C;
        int field_28;
        SPlayerStats *pStats;
        char bBlueTeam;
        char bCollide;
        char field_32;
        char field_33;
        CAnimMesh *pFpgunMesh;
        CAnimMesh *pLastFpgunMesh;
        Timer Timer3C;
        int FpgunMuzzleProps[2];
        int FpgunAmmoDigit1Prop;
        int FpgunAmmoDigit2Prop;
        int field_50[24];
        char field_B0;
        char IsCrouched;
        char field_B2;
        char field_B3;
        int hViewObj;
        Timer WeaponSwitchTimer2;
        Timer UseKeyTimer;
        Timer field_C0;
        Camera *pCamera;
        int xViewport;
        int yViewport;
        int cxViewport;
        int cyViewport;
        float fFov;
        int ViewMode;
        int field_E0;
        PlayerConfig Config;
        int field_F6C;
        int field_F70;
        int field_F74;
        int field_F78;
        int field_F7C;
        PlayerWeaponInfo WeaponInfo;
        int FpgunWeaponId;
        int field_1084;
        int ScannerBmHandle;
        int field_108C[2];
        Player_1094 field_1094;
        int field_10C4[3];
        Color HitScrColor;
        int HitScrAlpha;
        int field_10D8;
        int field_10DC;
        int field_10E0;
        int field_10E4;
        int field_10E8[26];
        int Sound1150_NotSure;
        int PrefWeapons[32];
        float field_11D4;
        float field_11D8;
        void(__cdecl *field_11DC)(CPlayer *);
        float field_11E0[4];
        int field_11F0;
        float field_11F4;
        int field_11F8;
        int field_11FC;
        CPlayerNetData *pNwData;
    };
    static_assert(sizeof(CPlayer) == 0x1204, "invalid size");

    static auto &g_pPlayersList = *(CPlayer**)0x007C75CC;
    static auto &g_pLocalPlayer = *(CPlayer**)0x007C75D4;

    static const auto KillLocalPlayer = (void(*)())0x004757A0;
    static const auto HandleCtrlInGame = (void(*)(CPlayer *pPlayer, EGameCtrl KeyId, char WasPressed))0x004A6210;
    static const auto IsEntityCtrlActive = (char(*)(ControlConfig *pCtrlConf, EGameCtrl CtrlId, bool *pWasPressed))0x0043D4F0;
    static const auto PlayerCreateEntity = (EntityObj *(*)(CPlayer *pPlayer, int ClassId, const CVector3 *pPos, const CMatrix3 *pRotMatrix, int MpCharacter))0x004A4130;

    typedef bool(*IsPlayerEntityInvalid_Type)(CPlayer *pPlayer);
    static const auto IsPlayerEntityInvalid = (IsPlayerEntityInvalid_Type)0x004A4920;

    typedef bool(*IsPlayerDying_Type)(CPlayer *pPlayer);
    static const auto IsPlayerDying = (IsPlayerDying_Type)0x004A4940;

    /* Player Fpgun */

    static const auto PlayerFpgunRender = (void(*)(CPlayer*))0x004A2B30;
    static const auto PlayerFpgunUpdate = (void(*)(CPlayer*))0x004A2700;
    static const auto PlayerFpgunSetupMesh = (void(*)(CPlayer*, int WeaponClsId))0x004AA230;
    static const auto PlayerFpgunUpdateState = (void(*)(CPlayer*))0x004AA3A0;
    static const auto PlayerFpgunUpdateMesh = (void(*)(CPlayer*))0x004AA6D0;
    static const auto PlayerRenderRocketLauncherScannerView = (void(*)(CPlayer *pPlayer))0x004AEEF0;
    static const auto PlayerFpgunSetState = (void(*)(CPlayer *pPlayer, int State))0x004AA560;
    static const auto PlayerFpgunHasState = (bool(*)(CPlayer *pPlayer, int State))0x004A9520;

    static const auto IsEntityLoopFire = (bool(*)(int hEntity, signed int WeaponClsId))0x0041A830;
    static const auto EntityIsSwimming = (bool(*)(EntityObj *pEntity))0x0042A0A0;
    static const auto EntityIsFalling = (bool(*)(EntityObj *pEntit))0x0042A020;
    static const auto EntityIsReloading = (bool(*)(EntityObj *pEntity))0x00425250;

    /* Object */

    enum EObjectType
    {
        OT_ENTITY = 0x0,
        OT_ITEM = 0x1,
        OT_WEAPON = 0x2,
        OT_DEBRIS = 0x3,
        OT_CLUTTER = 0x4,
        OT_TRIGGER = 0x5,
        OT_EVENT = 0x6,
        OT_CORPSE = 0x7,
        OT_KEYFRAME = 0x8,
        OT_MOVER = 0x9,
        OT_CORONA_10 = 0xA,
    };

    struct SPosRotUnk
    {
        int field_88;
        float field_8C;
        int field_90;
        int field_94;
        float fMass;
        CMatrix3 field_9C;
        CMatrix3 field_C0;
        CVector3 Pos;
        CVector3 vNewPos;
        CMatrix3 matYawRot;
    };

    struct SWaterSplashUnk
    {
        CVector3 field_1B4;
        CVector3 field_1C0;
        float field_1CC;
        int field_1D0;
        float field_1D4;
        CVector3 field_1D8;
        int hUnkEntity;
        int field_1E8;
        int bWaterSplash_1EC;
        int field_3C;
        int field_40;
    };

    struct PhysicsInfo
    {
        float fElasticity;
        float field_8C;
        float fFriction;
        int field_94;
        float fMass;
        CMatrix3 field_9C;
        CMatrix3 field_C0;
        CVector3 vPos;
        CVector3 vNewPos;
        CMatrix3 matYawRot;
        CMatrix3 field_120;
        CVector3 vVel;
        CVector3 vRotChangeUnk;
        CVector3 field_15C;
        CVector3 field_168;
        CVector3 RotChangeUnkDelta;
        float field_180;
        DynamicArray field_184;
        CVector3 field_190;
        CVector3 field_19C;
        PhysicsFlags Flags;
        int FlagsSplash_1AC;
        int field_1B0;
        SWaterSplashUnk WaterSplashUnk;
    };
    static_assert(sizeof(PhysicsInfo) == 0x170, "invalid size");

    struct Object
    {
        void *pRoom;
        CVector3 vLastPosInRoom;
        Object *pNextObj;
        Object *pPrevObj;
        CString strName;
        int Uid;
        EObjectType Type;
        int Team;
        int Handle;
        int hOwnerEntityUnk;
        float fLife;
        float fArmor;
        CVector3 vPos;
        CMatrix3 matOrient;
        CVector3 vLastPos;
        float fRadius;
        ObjectFlags Flags;
        CAnimMesh *pAnimMesh;
        int field_84;
        PhysicsInfo PhysInfo;
        int Friendliness;
        int iMaterial;
        int hParent;
        int UnkPropId_204;
        CVector3 field_208;
        CMatrix3 mat214;
        CVector3 vPos3;
        CMatrix3 matRot2;
        int *pEmitterListHead;
        int field_26C;
        char KillerId;
        char _pad[3]; // FIXME
        int MultiHandle;
        int pAnim;
        int field_27C;
        CVector3 field_280;
    };
    static_assert(sizeof(Object) == 0x28C, "invalid size");

    struct EventObj
    {
        int pVtbl;
        Object _Super;
        int EventType;
        float fDelay;
        int field_298;
        int LinkList;
        int field_2A0;
        int field_2A4;
        int field_2A8;
        int field_2AC;
        int field_2B0;
    };

    struct ItemObj
    {
        Object _Super;
        ItemObj *pNext;
        ItemObj *pPrev;
        int field_294;
        int ItemClsId;
        CString field_29C;
        int field_2A4;
        int field_2A8;
        int field_2AC;
        char field_2B0[12];
        int field_2BC;
        int field_2C0;
    };

    static auto &g_ItemObjList = *(ItemObj*)0x00642DD8;

    static const auto ObjGetFromUid = (Object *(*)(int Uid))0x0048A4A0;

    /* Entity */

    typedef void EntityClass;

    struct MoveModeTbl
    {
        char bAvailable;
        char unk, unk2, unk3;
        int Id;
        int iMoveRefX;
        int iMoveRefY;
        int iMoveRefZ;
        int iRotRefX;
        int iRotRefY;
        int iRotRefZ;
    };

    struct SWeaponSelection
    {
        EntityClass *pEntityCls;
        int WeaponClsId;
        int WeaponClsId2;
    };

    struct SEntityMotion
    {
        CVector3 vRotChange;
        CVector3 vPosChange;
        int field_18;
        float field_1C;
        float field_20;
    };

    struct EntityWeapon_2E8_InnerUnk
    {
        CVector3 field_0;
        CVector3 field_C;
        int field_18;
        int field_1C;
        float field_20;
        int field_24;
        DynamicArray field_28;
        char field_34;
        char field_35;
        __int16 field_36;
        int field_38;
        int field_3C;
        int field_40;
        int field_44;
        CMatrix3 field_48;
        int field_6C;
        DynamicArray field_70;
    };

    struct EntityWeapon_2E8
    {
        int field_0;
        int field_4;
        int field_8[5];
        EntityWeapon_2E8_InnerUnk field_1C;
        EntityWeapon_2E8_InnerUnk field_98;
        int field_114;
        int field_118;
        Timer Timer_11C;
        int field_120;
        int field_124;
        int field_128;
        int field_12C;
        int hEntityUnk;
        Timer Timer_134;
        CVector3 field_138;
        int field_144;
        int field_148;
        CVector3 field_14C;
        int field_158;
        int field_15C;
        int field_160;
        CVector3 field_164;
        int field_170;
        CVector3 field_174;
    };

    struct EntityWeaponInfo
    {
        EntityObj *pEntity;
        int WeaponClsId;
        int WeaponClsId2;
        int ClipAmmo[32];
        int WeaponsAmmo[64];
        char field_18C[64];
        char field_1CC[64];
        DynamicArray field_20C;
        Timer FireWaitTimer;
        Timer field_4BC;
        Timer ImpactDelayTimer[2];
        int field_228;
        Timer Timer_22C;
        Timer Timer_230;
        Timer Timer_234;
        Timer Timer_238;
        Timer Timer_23C;
        CVector3 field_240;
        int field_24C;
        Timer Timer_250;
        Timer Timer_254;
        Timer Timer_258;
        Timer Timer_25C;
        int field_260;
        Timer Timer_264;
        int field_268;
        int field_26C;
        int field_270;
        Timer UnkWeaponTimer;
        Timer field_518;
        int field_51C;
        int field_520;
        int field_524;
        int UnkTime288;
        int field_28C;
        int field_290;
        Timer Timer_294;
        float fCreateTime;
        char field_29C;
        char field_29D;
        __int16 field_29E;
        Timer Timer_2A0;
        Timer Timer_2A4;
        Timer Timer_2A8;
        int field_2AC[5];
        int UnkObjHandle;
        CVector3 field_2C4;
        Timer field_2D0;
        int field_2D4;
        int field_2D8;
        Timer field_2DC;
        Timer field_2E0;
        int field_2E4;
        EntityWeapon_2E8 field_2E8;
        SEntityMotion MotionChange;
        Timer field_48C;
        CVector3 field_490;
        float fLastDmgTime;
        int field_4A0;
        Timer field_4A4;
        int field_4A8[3];
        Timer field_4B4;
        Timer field_4B8;
        int UnkHandle;
        int field_4C0;
        Timer field_4C4;
        int field_4C8;
        int field_4CC;
        Timer field_4D0;
        Timer field_4D4;
        Timer field_4D8;
        Timer field_4DC;
        Timer field_4E0;
        Timer field_4E4;
        Timer field_4E8;
        CVector3 field_4EC;
        Timer Timer_4F8;
        Timer Timer_4FC;
        CVector3 field_500;
        int field_50C;
        float LastRotTime;
        float LastMoveTime;
        float LastFireLevelTime;
        int field_51C_;
        float fMovementRadius;
        float field_524_;
        int field_528;
        int field_52C;
        int Flags;
    };
    static_assert(sizeof(EntityWeaponInfo) == 0x534, "invalid size");

    struct EntityCameraInfo
    {
        int field_860;
        CVector3 vRotYaw;
        CVector3 vRotYawDelta;
        CVector3 vRotPitch;
        CVector3 vRotPitchDelta;
        CVector3 field_894;
        CVector3 field_8A0;
        int field_8AC;
        int field_8B0;
        float CameraShakeDist;
        float CameraShakeTime;
        Timer CameraShakeTimer;
    };

    struct EntityObj
    {
        Object _Super;
        EntityObj *pNext;
        EntityObj *pPrev;
        EntityClass *pClass;
        int ClassId;
        EntityClass *pClass2;
        EntityWeaponInfo WeaponInfo;
        CVector3 vViewPos;
        CMatrix3 matViewOrient;
        int UnkAmbientSound;
        int field_808;
        int MoveSnd;
        EntityFlags EntityFlags;
        EntityPowerups Powerups;
        Timer UnkTimer818;
        int LaunchSnd;
        int field_820;
        __int16 KillType;
        __int16 field_826;
        int field_828;
        int field_82C;
        Timer field_830;
        int field_834[5];
        CAnimMesh *field_848;
        CAnimMesh *field_84C;
        int pNanoShield;
        int Snd854;
        MoveModeTbl *MovementMode;
        CMatrix3 *field_85C;
        EntityCameraInfo CameraInfo;
        float field_8C0;
        int field_8C4;
        int field_8C8;
        DynamicArray InterfaceProps;
        DynamicArray UnkAnimMeshArray8D8;
        int field_8E4[92];
        int UnkAnims[180];
        int field_D24[129];
        int field_F28[200];
        int field_1248[6];
        int field_1260;
        int field_1264;
        int field_1268[65];
        Timer Timer_136C;
        Timer Timer1370;
        Timer ShellEjectTimer;
        int field_1378;
        Timer field_137C;
        int field_1380;
        int field_1384;
        int field_1388;
        int field_138C;
        int field_1390;
        float field_1394;
        float field_1398;
        Timer field_139C;
        Timer field_13A0;
        Timer WeaponDrainTimer;
        int ReloadAmmo;
        int ReloadClipAmmo;
        int ReloadWeaponClsId;
        float field_13B4;
        CVector3 field_13B8;
        int field_13C4[2];
        Timer field_13CC;
        int field_13D0[2];
        int field_13D8;
        int field_13DC;
        CVector3 field_13E0;
        int field_13EC;
        int field_13F0;
        int UnkBoneId13F4;
        void *field_13F8;
        Timer Timer13FC;
        int SpeakerSound;
        int field_1404;
        float fUnkCountDownTime1408;
        Timer UnkTimer140C;
        CAnimMesh *field_1410;
        Timer SplashInCounter;
        DynamicArray field_1418;
        int field_1424;
        int field_1428;
        int field_142C;
        CPlayer *pLocalPlayer;
        CVector3 vPitchMin;
        CVector3 vPitchMax;
        int hKillerEntity;
        int RiotShieldClutterHandle;
        int field_1454;
        Timer field_1458;
        int UnkClutterHandles[2];
        float fTime;
        int field_1468;
        int hUnkEntity;
        int field_1470;
        Color AmbientColor;
        int field_1478;
        int field_147C;
        int field_1480;
        int field_1484;
        int field_1488;
        CAnimMesh *RespawnVfx;
        Timer field_1490;
    };
    static_assert(sizeof(EntityObj) == 0x1494, "invalid size");

    typedef EntityObj *(*EntityGetFromHandle_Type)(uint32_t hEntity);
    static const auto EntityGetFromHandle = (EntityGetFromHandle_Type)0x00426FC0;

    /* Weapons */

    struct SWeaponStateAction
    {
        CString strName;
        CString strAnim;
        int iAnim;
        int SoundId;
        int pSound;
    };

    struct WeaponClass
    {
        CString strName;
        CString strDisplayName;
        CString strV3dFilename;
        int dwV3dType;
        CString strEmbeddedV3dFilename;
        int AmmoType;
        CString str3rdPersonV3d;
        CAnimMesh *p3rdPersonMeshNotSure;
        int Muzzle1PropId;
        int ThirdPersonGrip1MeshProp;
        int dw3rdPersonMuzzleFlashGlare;
        CString str1stPersonMesh;
        int *pMesh;
        CVector3 v1stPersonOffset;
        CVector3 v1stPersonOffsetSS;
        int p1stPersonMuzzleFlashBitmap;
        float f1stPersonMuzzleFlashRadius;
        int p1stPersonAltMuzzleFlashBitmap;
        float f1stPersonAltMuzzleFlashRadius;
        float f1stPersonFov;
        float g_fSplitscreenFov;
        int cProjectiles;
        int ClipSizeSP;
        int ClipSizeMP;
        int ClipSize;
        float fClipReloadTime;
        float fClipDrainTime;
        int Bitmap;
        int TrailEmitter;
        float fHeadRadius;
        float fTailRadius;
        float fHeadLen;
        int IsSecondary;
        float fCollisionRadius;
        int fLifetime;
        float fLifetimeSP;
        float fLifetimeMulti;
        float fMass;
        float fVelocity;
        float fVelocityMulti;
        float fVelocitySP;
        float fLifetimeMulVel;
        float fFireWait;
        float fAltFireWait;
        float fSpreadDegreesSP;
        float fSpreadDegreesMulti;
        int fSpreadDegrees;
        float fAltSpreadDegreesSP;
        float fAltSpreadDegreesMulti;
        int fAltSpreadDegrees;
        float fAiSpreadDegreesSP;
        float fAiSpreadDegreesMulti;
        int fAiSpreadDegrees;
        float fAiAltSpreadDegreesSP;
        float fAiAltSpreadDegreesMulti;
        int fAiAltSpreadDegrees;
        float fDamage;
        float fDamageMulti;
        float fAltDamage;
        float fAltDamageMulti;
        float fAIDmgScaleSP;
        float fAIDmgScaleMP;
        int fAIDmgScale;
        int field_124;
        int field_128;
        int field_12C;
        float fTurnTime;
        float fViewCone;
        float fScanningRange;
        float fWakeupTime;
        float fDrillTime;
        float fDrillRange;
        float fDrillCharge;
        float fCraterRadius;
        float ImpactDelay[2];
        float AltImpactDelay[2];
        int Launch;
        int AltLaunch;
        int SilentLaunch;
        int UnderwaterLaunch;
        int LaunchFail;
        int FlySound;
        int MaterialImpactSound[30];
        int NearMissSnd;
        int NearMissUnderwaterSnd;
        int GeomodSound;
        int StartSound;
        float fStartDelay;
        int StopSound;
        float fDamageRadius;
        float fDamageRadiusSP;
        float fDamageRadiusMulti;
        float Glow;
        float field_218;
        int Glow2;
        int field_220;
        int field_224;
        int field_228;
        int MuzzleFlashLight;
        int field_230;
        int MuzzleFlashLight2;
        int field_238;
        int field_23C;
        int field_240;
        int HudIcon;
        int HudReticleTexture;
        int HudZoomedReticleTexture;
        int HudLockedReticleTexture;
        int ZoomSound;
        int cMaxAmmoSP;
        int cMaxAmmoMP;
        int cMaxAmmo;
        int Flags;
        int Flags2;
        int field_26C;
        int field_270;
        int TracerFrequency;
        int field_278;
        float fPiercingPower;
        float RicochetAngle;
        int RicochetBitmap;
        CVector3 vRicochetSize;
        float fThrustLifetime;
        int cStates;
        int cActions;
        SWeaponStateAction States[3];
        SWeaponStateAction Actions[7];
        int field_3B8[35];
        int BurstCount;
        float fBurstDelay;
        int BurstLaunchSound;
        int ImpactVclipsCount;
        int ImpactVclips[3];
        CString ImpactVclipNames[3];
        float ImpactVclipsRadiusList[3];
        int DecalTexture;
        CVector3 vDecalSize;
        int GlassDecalTexture;
        CVector3 vGlassDecalSize;
        int FpgunShellEjectPropId;
        CVector3 vShellsEjectedBaseDir;
        float fShellEjectVelocity;
        CString strShellsEjectedV3d;
        int ShellsEjectedCustomSoundSet;
        float fPrimaryPauseTimeBeforeEject;
        int FpgunClipEjectPropId;
        CString strClipsEjectedV3d;
        float fClipsEjectedDropPauseTime;
        int ClipsEjectedCustomSoundSet;
        float fReloadZeroDrain;
        float fCameraShakeDist;
        float fCameraShakeTime;
        CString strSilencerV3d;
        int field_4F0_always0;
        int FpgunSilencerPropId;
        CString strSparkVfx;
        int field_500_always0;
        int FpgunThrusterPropId;
        int field_508;
        float fAIAttackRange1;
        float fAIAttackRange2;
        float field_514;
        int FpgunWeaponPropId;
        int field_51C_always1neg;
        int WeaponType;
        CString strWeaponIcon;
        int DamageType;
        int CyclePos;
        int PrefPos;
        float fFineAimRegSize;
        float fFineAimRegSizeSS;
        CString strTracerEffect;
        int field_548_always0;
        float fMultiBBoxSizeFactor;
    };
    static_assert(sizeof(WeaponClass) == 0x550, "invalid size");

    static const auto g_pWeaponClasses = (WeaponClass*)0x0085CD08;
    static auto &g_RiotStickClsId = *(uint32_t*)0x00872468;
    static auto &g_RemoteChargeClsId = *(uint32_t*)0x0087210C;

    /* Window */

    typedef void(*PFN_MESSAGE_HANDLER)(UINT, WPARAM, LPARAM);
    typedef unsigned(*PFN_ADD_MSG_HANDLER)(PFN_MESSAGE_HANDLER);
    static const auto AddMsgHandler = (PFN_ADD_MSG_HANDLER)0x00524AE0;

    static const auto g_MsgHandlers = (PFN_MESSAGE_HANDLER*)0x01B0D5A0;
    static auto &g_cMsgHandlers = *(uint32_t*)0x01B0D760;

    static auto &g_hWnd = *(HWND*)0x01B0D748;
    static auto &g_bIsActive = *(uint8_t*)0x01B0D750;
    static auto &g_bClose = *(uint8_t*)0x01B0D758;
    static auto &g_bMouseInitialized = *(uint8_t*)0x01885461;
    static auto &g_cRedrawServer = *(uint32_t*)0x01775698;

    /* Network Game */

    struct BanlistEntry
    {
        char szIp[24];
        BanlistEntry *pNext;
        BanlistEntry *pPrev;
    };

    typedef unsigned(*GetGameType_Type)();
    static const auto GetGameType = (GetGameType_Type)0x00470770;

    typedef unsigned(*GetPlayersCount_Type)();
    static const auto GetPlayersCount = (GetPlayersCount_Type)0x00484830;

    typedef uint8_t(*GetTeamScore_Type)();
    static const auto CtfGetRedScore = (GetTeamScore_Type)0x00475020;
    static const auto CtfGetBlueScore = (GetTeamScore_Type)0x00475030;
    static const auto TdmGetRedScore = (GetTeamScore_Type)0x004828F0;
    static const auto TdmGetBlueScore = (GetTeamScore_Type)0x00482900;
    static const auto CtfGetRedFlagPlayer = (CPlayer*(*)())0x00474E60;
    static const auto CtfGetBlueFlagPlayer = (CPlayer*(*)())0x00474E70;

    typedef const char *(*GetJoinFailedStr_Type)(unsigned Reason);
    static const auto GetJoinFailedStr = (GetJoinFailedStr_Type)0x0047BE60;

    typedef void(*KickPlayer_Type)(CPlayer *pPlayer);
    static const auto KickPlayer = (KickPlayer_Type)0x0047BF00;

    typedef void(*BanIp_Type)(const NwAddr *pAddr);
    static const auto BanIp = (BanIp_Type)0x0046D0F0;

    static const auto MpResetNetGame = (void(*)())0x0046E450;
    static const auto MultiSetNextWeapon = (void(*)(int WeaponClsId))0x0047FCA0;

    static auto &g_ServAddr = *(NwAddr*)0x0064EC5C;
    static auto &g_strServName = *(CString*)0x0064EC28;
    static auto &g_bNetworkGame = *(uint8_t*)0x0064ECB9;
    static auto &g_bLocalNetworkGame = *(uint8_t*)0x0064ECBA;
    static auto &g_bDedicatedServer = *(uint32_t*)0x01B0D75C;
    static auto &g_GameOptions = *(uint32_t*)0x0064EC40;
    static auto &g_pBanlistFirstEntry = *(BanlistEntry**)0x0064EC20;
    static auto &g_pBanlistLastEntry = *(BanlistEntry**)0x0064EC24;
    static auto &g_BanlistNullEntry = *(BanlistEntry*)0x0064EC08;

    /* Input */
    static const auto MouseGetPos = (int(*)(int *pX, int *pY, int *pZ))0x0051E450;
    static const auto MouseWasButtonPressed = (int(*)(int BtnIdx))0x0051E5D0;

    /* Menu */

    enum MenuId
    {
        MENU_INIT = 0x1,
        MENU_MAIN = 0x2,
        MENU_EXTRAS = 0x3,
        MENU_INTRO_VIDEO = 0x4,
        MENU_LOADING_LEVEL = 0x5,
        MENU_SAVE_GAME = 0x6,
        MENU_LOAD_GAME = 0x7,
        MENU_8 = 0x8,
        MENU_9 = 0x9,
        MENU_GAME_DYING = 0xA,
        MENU_IN_GAME = 0xB,
        MENU_C = 0xC,
        MENU_EXIT_GAME = 0xD,
        MENU_OPTIONS = 0xE,
        MENU_MULTIPLAYER = 0xF,
        MENU_HELP = 0x10,
        MENU_11 = 0x11,
        MENU_IN_SPLITSCREEN_GAME = 0x12,
        MENU_GAME_OVER = 0x13,
        MENU_14 = 0x14,
        MENU_GAME_INTRO = 0x15,
        MENU_16 = 0x16,
        MENU_CREDITS = 0x17,
        MENU_BOMB_TIMER = 0x18,
        MENU_19 = 0x19,
        MENU_1A = 0x1A,
        MENU_1B = 0x1B,
        MENU_1C = 0x1C,
        MENU_1D = 0x1D,
        MENU_SERVER_LIST = 0x1E,
        MENU_MP_SPLITSCREEN = 0x1F,
        MENU_MP_CREATE_GAME = 0x20,
        MENU_21 = 0x21,
        MENU_MP_LIMBO = 0x22,
    };

    static const auto SwitchMenu = (int(*)(int MenuId, bool bForce))0x00434190;
    static const auto GetCurrentMenuId = (MenuId(*)())0x00434200;

    static auto &g_MenuLevels = *(int*)0x00630064;
    static auto &g_CurrentMenuLevel = *(int*)0x005967A4;
    static auto &g_MenuVersionLabel = *(UiPanel*)0x0063C088;

    /* Other */

    enum EGameLang
    {
        LANG_EN = 0,
        LANG_GR = 1,
        LANG_FR = 2,
    };

    struct RflLightmap
    {
        BYTE *pUnk;
        int w;
        int h;
        BYTE *pBuf;
        int BmHandle;
        int unk2;
    };

    static const auto g_pszRootPath = (char*)0x018060E8;
    static auto &g_fFps = *(float*)0x005A4018;
    static auto &g_fFramerate = *(float*)0x005A4014;
    static auto &g_fMinFramerate = *(float*)0x005A4024;
    static auto &g_SimultaneousPing = *(uint32_t*)0x00599CD8;
    static auto &g_strLevelName = *(CString*)0x00645FDC;
    static auto &g_strLevelFilename = *(CString*)0x00645FE4;
    static auto &g_strLevelAuthor = *(CString*)0x00645FEC;
    static auto &g_strLevelDate = *(CString*)0x00645FF4;
    static auto &g_BigFontId = *(int*)0x006C74C0;
    static auto &g_LargeFontId = *(int*)0x0063C05C;
    static auto &g_MediumFontId = *(int*)0x0063C060;
    static auto &g_SmallFontId = *(int*)0x0063C068;
    static auto &g_bDirectInputDisabled = *(bool*)0x005A4F88;
    static auto &g_bScoreboardRendered = *(bool*)0x006A1448;
    static auto &g_strDefaultPlayerWeapon = *(CString*)0x007C7600;

    static const auto g_pbDbgFlagsArray = (uint8_t*)0x0062FE19;
    static auto &g_bDbgNetwork = *(uint32_t*)0x006FED24;
    static auto &g_bDbgWeapon = *(uint8_t*)0x007CAB59;
    static auto &g_bRenderEventIcons = *(uint8_t*)0x00856500;
    static auto &g_bDbgRenderTriggers = *(uint8_t*)0x0085683C;

    static const auto RfBeep = (void(*)(unsigned u1, unsigned u2, unsigned u3, float fVolume))0x00505560;
    static const auto GetFileExt = (char *(*)(const char *pszPath))0x005143F0;
    static const auto SplitScreenStart = (void(*)())0x00480D30;
    static const auto SetNextLevelFilename = (void(*)(CString strFilename, CString strSecond))0x0045E2E0;
    static const auto DemoLoadLevel = (void(*)(const char *pszLevelFileName))0x004CC270;
    static const auto RenderReticle = (void(*)(CPlayer *pPlayer))0x0043A2C0;
    static const auto SetCursorVisible = (void(*)(char bVisible))0x0051E680;
    static const auto RflLoad = (int(*)(CString *pstrLevelFilename, CString *a2, char *pszError))0x0045C540;
    static const auto DrawScoreboard = (void(*)(bool bDraw))0x00470860;
    static const auto DrawScoreboardInternal = (void(*)(bool bDraw))0x00470880;
    static const auto BinkInitDeviceInfo = (unsigned(*)())0x005210C0;
    static const auto SndConvertVolume3D = (void(*)(int GameSndId, CVector3 *pSoundPos, float *pPanOut, float *pVolumeOut, float VolumeIn))0x00505740;


    /* Strings Table */
    static const auto g_ppszStringsTable = (char**)0x007CBBF0;
    enum StrId
    {
        STR_PLAYER = 675,
        STR_FRAGS = 676,
        STR_PING = 677,
        STR_CAPS = 681,
        STR_WAS_KILLED_BY_HIW_OWN_HAND = 693,
        STR_WAS_KILLED_BY = 694,
        STR_WAS_KILLED_MYSTERIOUSLY = 695,
        STR_SCORE = 720,
        STR_PLAYER_NAME = 835,
        STR_EXITING_GAME = 884,
        STR_USAGE = 886,
        STR_YOU_KILLED_YOURSELF = 942,
        STR_YOU_JUST_GOT_BEAT_DOWN_BY = 943,
        STR_YOU_WERE_KILLED_BY = 944,
        STR_YOU_KILLED = 945,
        STR_GOT_BEAT_DOWN_BY = 946,
        STR_KICKING_PLAYER = 958,
    };

    /* RF stdlib functions are not compatible with GCC */

    typedef FILE *(*RfFopen_Type)(const char *pszPath, const char *pszMode);
    static const auto RfFopen = (RfFopen_Type)0x00574FFE;

    typedef void(*RfSeek_Type)(FILE *pFile, uint32_t Offset, uint32_t Direction);
    static const auto RfSeek = (RfSeek_Type)0x00574F14;

    typedef void(*RfDelete_Type)(void *pMem);
    static const auto RfDelete = (RfDelete_Type)0x0057360E;

    typedef void *(*RfMalloc_Type)(uint32_t cbSize);
    static const auto RfMalloc = (RfMalloc_Type)0x00573B37;
}

#pragma pack(pop)
