#pragma once

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef BOOL WINBOOL;

#include <d3d8.h>

#pragma pack(push, 1)

namespace rf
{
    /* Declarations */
    struct CPlayer;
    struct EntityObj;

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

    constexpr auto StringAlloc = (char*(*)(unsigned cbSize))0x004FF300;
    constexpr auto StringFree = (void(*)(void *pData))0x004FF3A0;
    constexpr auto CString_Init = (CString*(__thiscall *)(CString *This, const char *pszInit))0x004FF3D0;
    constexpr auto CString_InitFromStr = (CString*(__thiscall *)(CString *This, const CString *pstrInit))0x004FF410;
    constexpr auto CString_InitEmpty = (CString*(__thiscall *)(CString *This))0x004FF3B0;
    constexpr auto CString_CStr = (const char*(__thiscall *)(CString *This))0x004FF480;
    constexpr auto CString_Destroy = (void(__thiscall *)(CString *This))0x004FF470;

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

    constexpr auto DcPrint = (void(*)(const char *pszText, const int *pColor))0x00509EC0;
    constexpr auto DcPrintf = (void(*)(const char *pszFormat, ...))0x0050B9F0;
    constexpr auto DcGetArg = (void(*)(int Type, int bUnknown))0x0050AED0;
    constexpr auto DcUpdate = (void(*)(BOOL bServer))0x0050A720;

    //constexpr auto g_ppDcCommands = (DcCommand**)0x01775530;
    constexpr auto g_pDcNumCommands = (uint32_t*)0x0177567C;

    constexpr auto g_pbDcRun = (uint32_t*)0x01775110;
    constexpr auto g_pbDcHelp = (uint32_t*)0x01775224;
    constexpr auto g_pDcArgType = (uint32_t*)0x01774D04;
    constexpr auto g_pszDcArg = (char*)0x0175462C;
    constexpr auto g_piDcArg = (int*)0x01775220;
    constexpr auto g_pfDcArg = (float*)0x01754628;

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

    constexpr auto g_ppDirect3D = (IDirect3D8**)0x01CFCBE0;
    constexpr auto g_ppGrDevice = (IDirect3DDevice8**)0x01CFCBE4;
    constexpr auto g_pGrScreen = (GrScreen*)0x017C7BC0;
    constexpr auto g_pGrGammaRamp = (uint32_t*)0x017C7C68;
    constexpr auto g_pAdapterIdx = (uint32_t*)0x01CFCC34;
    constexpr auto g_pGrScaleVec = (CVector3*)0x01818B48;
    constexpr auto g_GrViewMatrix = (CMatrix3*)0x018186C8;
    constexpr auto g_pGrDeviceCaps = (D3DCAPS8*)0x01CFCAC8;
    constexpr auto g_pGrDefaultWFar = (float*)0x00596140;

    constexpr auto g_pGrLineMaterial = (uint32_t*)0x01775B00;
    constexpr auto g_pGrRectMaterial = (uint32_t*)0x17756C0;
    constexpr auto g_pGrTextMaterial = (uint32_t*)0x17C7C5C;
    constexpr auto g_pGrBitmapMaterial = (uint32_t*)0x017756BC;
    constexpr auto g_pGrImageMaterial = (uint32_t*)0x017756DC;

    constexpr auto GrGetMaxWidth = (unsigned(*)())0x0050C640;
    constexpr auto GrGetMaxHeight = (unsigned(*)())0x0050C650;
    constexpr auto GrGetViewportWidth = (unsigned(*)())0x0050CDB0;
    constexpr auto GrGetViewportHeight = (unsigned(*)())0x0050CDC0;
    constexpr auto GrSetColor = (void(*)(unsigned r, unsigned g, unsigned b, unsigned a))0x0050CF80;
    constexpr auto GrSetColorPtr = (void(*)(uint32_t *pColor))0x0050D000;
    constexpr auto GrDrawRect = (void(*)(unsigned x, unsigned y, unsigned cx, unsigned cy, unsigned Material))0x0050DBE0;
    constexpr auto GrDrawImage = (void(*)(int BmHandle, int x, int y, int Material))0x0050D2A0;
    constexpr auto GrDrawBitmapStretched = (void(*)(int BmHandle, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, int a10, float a11, int a12))0x0050D250;
    constexpr auto GrDrawPoly = (void(*)(int Num, GrVertex **ppVertices, int Flags, int iMat))0x0050DF80;
    constexpr auto GrDrawText = (void(*)(unsigned x, unsigned y, const char *pszText, int Font, unsigned Material))0x0051FEB0;
    constexpr auto GrDrawAlignedText = (void(*)(GrTextAlignment Align, unsigned x, unsigned y, const char *pszText, int Font, unsigned Material))0x0051FE50;
    constexpr auto GrFitText = (CString *(*)(CString *pstrDest, CString Str, int cxMax))0x00471EC0;
    constexpr auto GrReadBackBuffer = (int(*)(int x, int y, int Width, int Height, void *pBuffer))0x0050DFF0;
    constexpr auto GrLoadFont = (int(*)(const char *pszFileName, int a2))0x0051F6E0;
    constexpr auto GrGetFontHeight = (unsigned(*)(int FontId))0x0051F4D0;
    constexpr auto GrGetTextWidth = (void(*)(int *pOutWidth, int *pOutHeight, const char *pszText, int TextLen, int FontId))0x0051F530;
    constexpr auto GrFlushBuffers = (void(*)())0x00559D90;
    constexpr auto GrSwapBuffers = (void(*)())0x0050CE20;
    constexpr auto GrSetViewMatrix = (void(*)(CMatrix3 *pMatRot, CVector3 *pPos, float fFov, int a4, int a5))0x00517EB0;
    constexpr auto GrSetViewMatrixD3D = (void(*)(CMatrix3 *pMatRot, CVector3 *pPos, float fFov, int a4, int a5))0x00547150;
    constexpr auto GrRenderLine = (char(*)(const CVector3 *pWorldPos1, const CVector3 *pWorldPos2, int Material))0x00515960;
    constexpr auto GrRenderSphere = (char(*)(const CVector3 *pvPos, float fRadius, int Material))0x00515CD0;
    constexpr auto GrLock = (int(*)(int BmHandle, int SectionIdx, SGrLockData *pData, int a4))0x0050E2E0;
    constexpr auto GrUnlock = (void(*)(SGrLockData *pData))0x0050E310;
    constexpr auto GrD3DSetTextureData = (int(*)(int Level, const BYTE *pSrcBits, const BYTE *pPallete, int cxBm, int cyBm, int PixelFmt, void *a7, int cxTex, int cyTex, IDirect3DTexture8 *pTextures))0x0055BA10;
    constexpr auto GrInitBuffers = (void(*)())0x005450A0;
    constexpr auto GrSetTextureMipFilter = (void(*)(int bLinear))0x0050E830;

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

    constexpr auto BmLoad = (int(*)(const char *pszFilename, int a2, BOOL a3))0x0050F6A0;
    constexpr auto BmCreateUserBmap = (int(*)(BmPixelFormat PixelFormat, int Width, int Height))0x005119C0;
    constexpr auto BmConvertFormat = (void(*)(void *pDstBits, BmPixelFormat DstPixelFmt, const void *pSrcBits, BmPixelFormat SrcPixelFmt, int NumPixels))0x0055DD20;
    constexpr auto BmGetBitmapSize = (void(*)(int BmHandle, int *pWidth, int *pHeight))0x00510630;
    constexpr auto BmGetFilename = (const char*(*)(int BmHandle))0x00511710;
    constexpr auto BmLock = (BmPixelFormat(*)(int BmHandle, BYTE **ppData, BYTE **ppPalette))0x00510780;
    constexpr auto BmUnlock = (void(*)(int BmHandle))0x00511700;

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

    constexpr auto UiMsgBox = (void(*)(const char *pszTitle, const char *pszText, void(*pfnCallback)(void), BOOL bInput))0x004560B0;
    constexpr auto UiCreateDialog = (void(*)(const char *pszTitle, const char *pszText, unsigned cButtons, const char **ppszBtnTitles, void **ppfnCallbacks, unsigned Unknown1, unsigned Unknown2))0x004562A0;
    constexpr auto UiGetElementFromPos = (int(*)(int x, int y, UiPanel **ppGuiList, signed int cGuiList))0x00442ED0;

    /* HUD */

#define HUD_POINTS_COUNT 48
    constexpr auto g_pHudPosData640 = (POINT*)0x00637868;
    constexpr auto g_pHudPosData800 = (POINT*)0x006373D0;
    constexpr auto g_pHudPosData1024 = (POINT*)0x00637230;
    constexpr auto g_pHudPosData1280 = (POINT*)0x00637560;
    constexpr auto g_pHudPosData = (POINT*)0x006376E8;

    typedef void(*ChatPrint_Type)(CString strText, unsigned ColorId, CString Prefix);
    constexpr auto ChatPrint = (ChatPrint_Type)0x004785A0;

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
        char *pszFileName;
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

    constexpr auto g_pcArchives = (uint32_t*)0x01BDB214;
    constexpr auto g_pArchives = (Packfile*)0x01BA7AC8;
#define VFS_LOOKUP_TABLE_SIZE 20713
    constexpr auto g_pVfsLookupTable = (PackfileLookupTable*)0x01BB2AC8;
    constexpr auto g_pbVfsIgnoreTblFiles = (uint8_t*)0x01BDB21C;

    typedef BOOL(*PackfileLoad_Type)(const char *pszFileName, const char *pszDir);
    constexpr auto PackfileLoad = (PackfileLoad_Type)0x0052C070;

    typedef Packfile *(*PackfileFindArchive_Type)(const char *pszFilename);
    constexpr auto PackfileFindArchive = (PackfileFindArchive_Type)0x0052C1D0;

    typedef uint32_t(*PackfileCalcFileNameChecksum_Type)(const char *pszFileName);
    constexpr auto PackfileCalcFileNameChecksum = (PackfileCalcFileNameChecksum_Type)0x0052BE70;

    typedef uint32_t(*PackfileAddToLookupTable_Type)(PackfileEntry *pPackfileEntry);
    constexpr auto PackfileAddToLookupTable = (PackfileAddToLookupTable_Type)0x0052BCA0;

    typedef uint32_t(*PackfileProcessHeader_Type)(Packfile *pPackfile, const void *pHeader);
    constexpr auto PackfileProcessHeader = (PackfileProcessHeader_Type)0x0052BD10;

    typedef uint32_t(*PackfileAddEntries_Type)(Packfile *pPackfile, const void *pBuf, unsigned cFilesInBlock, unsigned *pcAddedEntries);
    constexpr auto PackfileAddEntries = (PackfileAddEntries_Type)0x0052BD40;

    typedef uint32_t(*PackfileSetupFileOffsets_Type)(Packfile *pPackfile, unsigned DataOffsetInBlocks);
    constexpr auto PackfileSetupFileOffsets = (PackfileSetupFileOffsets_Type)0x0052BEB0;

    constexpr auto FsAddDirectoryEx = (int(*)(const char *pszDir, const char *pszExtList, char bUnknown))0x00514070;

    constexpr auto FileGetChecksum = (unsigned(*)(const char *pszFilename))0x00436630;

    /* Network */

    struct NwAddr
    {
        uint32_t IpAddr;
        uint32_t Port;
    };

    constexpr auto g_pNwSocket = (SOCKET*)0x005A660C;

    typedef void(*NwProcessGamePackets_Type)(const char *pData, int cbData, const NwAddr *pAddr, CPlayer *pPlayer);
    constexpr auto NwProcessGamePackets = (NwProcessGamePackets_Type)0x004790D0;

    typedef void(*NwSendNotReliablePacket_Type)(const void *pAddr, const void *pPacket, unsigned cbPacket);
    constexpr auto NwSendNotReliablePacket = (NwSendNotReliablePacket_Type)0x0052A080;

    constexpr auto NwSendReliablePacket = (void(*)(CPlayer *pPlayer, const BYTE *pData, unsigned int cbData, int a4))0x00479480;

    typedef void(*NwAddrToStr_Type)(char *pszDest, int cbDest, NwAddr *pAddr);
    constexpr auto NwAddrToStr = (NwAddrToStr_Type)0x00529FE0;

    /* Player */

    struct SPlayerStats
    {
        uint16_t field_0;
        int16_t iScore;
        int16_t cCaps;
    };

    struct CPlayerNetData
    {
        NwAddr Addr;
        uint32_t Flags;
        uint32_t field_C;
        uint32_t field_10;
        uint32_t PlayerId;
        uint32_t field_18;
        char field_1C[1424];
        uint32_t dwPing;
        uint32_t field_5B0;
        char PacketBuf[512];
        uint32_t cbPacketBuf;
        uint32_t field_7B8[129];
        uint32_t ConnectionSpeed;
        uint32_t field_9C0;
        uint32_t field_9C4;
    };

    struct SKeyState
    {
        short DefaultScanCodes[2];
        short DefaultMouseBtnId;
        short field_6;
        int field_8;
        CString strName;
        short ScanCodes[2];
        short MouseBtnId;
        short field_1A;
    };

    struct CGameControls
    {
        float fMouseSensitivity;
        int bMouseLook;
        int field_EC;
        SKeyState Keys[128];
        int CtrlCount;
        int field_EF4;
        int field_EF8;
        int field_EFC;
        int field_F00;
        int field_F04;
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
    
    struct SPlayerSettings
    {
        CGameControls Controls;
        int field_F2C;
        int field_F30;
        BYTE field_F34[4];
        char field_F38;
        char field_F39;
        char field_F3A;
        char bHudVisible;
        char field_F3C;
        char field_E59;
        char field_E5A;
        char field_E5B;
        char field_F40;
        char field_F41;
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

    struct PlayerCamera
    {
        EntityObj *pCameraEntity;
        CPlayer *pPlayer;
        enum { CAM_1ST_PERSON, CAM_3RD_PERSON, CAM_FREE } Type;
    };

    struct CAnimMesh;

    struct CPlayer
    {
        CPlayer *pNext;
        CPlayer *pPrev;
        CString strName;
        int FireFlags;
        int hEntity;
        int EntityClsId;
        CVector3 field_1C;
        int field_28;
        SPlayerStats *pStats;
        char bBlueTeam;
        char bCollide;
        char field_32;
        char field_33;
        CAnimMesh *pWeaponMesh;
        int field_38;
        int field_3C;
        int field_40;
        int field_44;
        int field_48;
        int field_4C;
        int field_50;
        int field_54;
        int field_58;
        int field_5C;
        int field_60;
        int field_64;
        int field_68;
        int field_6C;
        int field_70;
        int field_74;
        int field_78;
        int field_7C;
        int field_80;
        int field_84;
        int field_88;
        int field_8C;
        int field_90;
        int field_94;
        int field_98;
        int field_9C;
        int field_A0;
        int field_A4;
        int field_A8;
        int field_AC;
        char field_B0;
        char IsCrouched;
        char field_B2;
        char field_B3;
        int hViewObj;
        int field_B8;
        int field_BC;
        int field_C0;
        PlayerCamera *pCamera;
        int xViewport;
        int yViewport;
        int cxViewport;
        int cyViewport;
        float fFov;
        int ViewMode;
        int field_E0;
        SPlayerSettings Settings;
        int field_F6C;
        int field_F70;
        int field_F74;
        int field_F78;
        int field_F7C;
        int NextWeaponClsId;
        int WeaponSwitchCounter;
        int field_F88;
        int field_F8C;
        int UnkTimerF90;
        char bInZoomMode;
        char field_F95;
        char field_F96;
        char field_F97;
        float field_F98;
        int field_F9C;
        int field_FA0;
        int field_FA4;
        int field_FA8;
        int field_FAC;
        char bRailgunScanner;
        char bScannerView;
        char clrUnkR;
        char clrUnkG;
        char clrUnkB;
        char clrUnkA;
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
        int field_1034_BoneId;
        int field_1038;
        int field_103C;
        int field_1040;
        int RemoteChargeVisible;
        int field_1048;
        int field_104C;
        int field_1050;
        int field_1054;
        int field_1058;
        int field_105C;
        int field_1060;
        int field_1064;
        int field_1068;
        int field_106C;
        int field_1070;
        int field_1074;
        int field_1078;
        int field_107C;
        int field_1080;
        int field_1084;
        int RailgunScannerBmHandle;
        int field_108C[17];
        int HitScrColor;
        int HitScrTime;
        float field_10D8;
        float field_10DC;
        int field_10E0;
        int field_10E4;
        int field_10E8;
        int field_10EC[26];
        int PrefWeapons[32];
        float field_11D4;
        float field_11D8;
        void(__cdecl *field_11DC)(CPlayer *);
        int field_11E0;
        int field_11E4[7];
        CPlayerNetData *pNwData;
    };

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

    constexpr auto g_ppPlayersList = (CPlayer**)0x007C75CC;
    constexpr auto g_ppLocalPlayer = (CPlayer**)0x007C75D4;

    constexpr auto PlayerCreate = (CPlayer *(*)(char bLocal))0x004A3310;
    constexpr auto PlayerDestroy = (void(*)(CPlayer *pPlayer))0x004A35C0;
    constexpr auto KillLocalPlayer = (void(*)())0x004757A0;
    constexpr auto HandleCtrlInGame = (void(*)(CPlayer *pPlayer, EGameCtrl KeyId, char WasPressed))0x004A6210;
    constexpr auto IsEntityCtrlActive = (char(*)(CGameControls *pControlsState, EGameCtrl CtrlId, bool *pWasPressed))0x0043D4F0;
    constexpr auto PlayerCreateEntity = (EntityObj *(*)(CPlayer *pPlayer, int ClassId, const CVector3 *pPos, const CMatrix3 *pRotMatrix, int MpCharacter))0x004A4130;

    typedef bool(*IsPlayerEntityInvalid_Type)(CPlayer *pPlayer);
    constexpr auto IsPlayerEntityInvalid = (IsPlayerEntityInvalid_Type)0x004A4920;

    typedef bool(*IsPlayerDying_Type)(CPlayer *pPlayer);
    constexpr auto IsPlayerDying = (IsPlayerDying_Type)0x004A4940;

    /* Player Fpgun */

    constexpr auto PlayerFpgunRender = (void(*)(CPlayer*))0x004A2B30;
    constexpr auto PlayerFpgunSetupMesh = (void(*)(CPlayer*, int WeaponClsId))0x004AA230;
    constexpr auto PlayerFpgunUpdate = (void(*)(CPlayer*))0x004A2700;
    constexpr auto PlayerFpgunUpdateMesh = (void(*)(CPlayer*))0x004AA6D0;

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

    struct Object
    {
        int field_0;
        CVector3 field_4;
        Object *pNextObj;
        Object *pPrevObj;
        CString strName;
        int Uid;
        EObjectType Type;
        int Team;
        int Handle;
        int unk2;
        float fLife;
        float fArmor;
        CVector3 vPos;
        CMatrix3 matRot;
        CVector3 vPos2;
        float fRadius;
        int Flags;
        CAnimMesh *pAnimMesh;
        int field_84;
        SPosRotUnk PosRotUnk;
        CMatrix3 field_120;
        CVector3 vVel;
        CVector3 vRotChangeUnk;
        CVector3 field_15C;
        CVector3 field_168;
        CVector3 RotChangeUnkDelta;
        float field_180;
        int field_184;
        int field_188;
        int field_18C;
        CVector3 field_190;
        CVector3 field_19C;
        int MovementFlags;
        int FlagsSplash_1AC;
        int field_1B0;
        SWaterSplashUnk WaterSplashUnk;
        int field_1F8;
        int iMaterial;
        int hParent;
        int field_204;
        CVector3 field_208;
        CMatrix3 field_214;
        CVector3 vPos3;
        CMatrix3 matRot2;
        int field_268;
        int field_26C;
        int KillerId;
        int MpHandle;
        int pAnim;
        int field_27C;
        int field_280;
        int field_284;
        int field_288;
    };

    struct EventObj
    {
        int unk0;
        Object Head;
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
        Object Head;
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

    constexpr auto *g_pItemObjList = (ItemObj*)0x00642DD8;

    constexpr auto ObjGetFromUid = (Object *(*)(int Uid))0x0048A4A0;

    /* Entity */

    typedef void EntityClass;

    struct SMovementMode
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

    struct EntityObj
    {
        Object Head;
        EntityObj *pNext;
        EntityObj *pPrev;
        EntityClass *pClass;
        int ClassId;
        EntityClass *pClass2;
        SWeaponSelection WeaponSel;
        int Ammo[8];
        char field_2CC[492];
        int field_4B8;
        char field_4BC[100];
        int field_520;
        char field_524[164];
        int field_5C8[80];
        SEntityMotion MotionChange;
        int field_72C[12];
        int UnkHandle;
        int field_760[20];
        float LastRotTime;
        float LastMoveTime;
        int field_7B8;
        int field_7BC[5];
        int field_7D0;
        CVector3 vWeaponPos;
        CMatrix3 matWeaponRot;
        int field_804;
        int field_808;
        int MoveSnd;
        int field_810_Flags;
        int field_814;
        int UnkTimer818;
        int LaunchSnd;
        int field_820;
        __int16 KillType;
        __int16 field_826;
        int field_828[8];
        CAnimMesh *field_848;
        int field_84C;
        int field_850;
        int field_854;
        SMovementMode *MovementMode;
        CMatrix3 *field_85C;
        int field_860;
        CVector3 vRotYaw;
        CVector3 vRotYawDelta;
        CVector3 vRotPitch;
        CVector3 vRotPitchDelta;
        CVector3 field_894;
        CVector3 field_8A0;
        int field_8AC[5];
        float field_8C0;
        int field_8C4;
        int field_8C8;
        int field_8CC;
        int field_8D0;
        int field_8D4;
        int field_8D8;
        int field_8DC;
        int field_8E0;
        int field_8E4[96];
        int field_A64[104];
        int field_C04;
        int field_C08[55];
        int UnkAnimIdxCE4;
        int field_CE8;
        int field_CEC;
        int field_CF0;
        int UnkAnimIdxCF4;
        int field_CF8[140];
        int field_F28[200];
        int field_1248[6];
        int field_1260;
        int field_1264;
        int field_1268[67];
        int UnkTimer1374;
        int field_1378[3];
        int field_1384;
        int field_1388;
        int field_138C;
        int field_1390;
        float field_1394;
        float field_1398;
        int field_139C[5];
        int field_13B0;
        int field_13B4;
        CVector3 field_13B8;
        int field_13C4[7];
        CVector3 field_13E0;
        int field_13EC;
        int field_13F0;
        int UnkBoneId13F4;
        void *field_13F8;
        int field_13FC;
        int field_1400;
        int field_1404;
        float fUnkCountDownTime1408;
        int UnkTimer140C;
        int field_1410;
        int SplashInCounter;
        char field_1418[16];
        int field_1428;
        int field_142C;
        CPlayer *pLocalPlayer;
        CVector3 vPitchMin;
        CVector3 vPitchMax;
        int field_144C[6];
        float fTime;
        int field_1468;
        int hUnkEntity;
        int field_1470;
        int AmbientColor;
        int field_1478;
        int field_147C;
        int field_1480;
        int field_1484;
        int field_1488;
        CAnimMesh *RespawnVfx;
        int field_1490;
    };

    typedef EntityObj *(*HandleToEntity_Type)(uint32_t hEntity);
    constexpr auto HandleToEntity = (HandleToEntity_Type)0x00426FC0;

    /* Weapons */

    struct WeaponClass
    {
        CString strName;
        CString strDisplayName;
        BYTE Rest[0x550 - 2 * sizeof(CString)];
    };

    constexpr auto g_pWeaponClasses = (WeaponClass*)0x0085CD08;
    constexpr auto g_pRiotStickClsId = (uint32_t*)0x00872468;
    constexpr auto g_pRemoteChargeClsId = (uint32_t*)0x0087210C;

    /* Window */

    typedef void(*PFN_MESSAGE_HANDLER)(UINT, WPARAM, LPARAM);
    typedef unsigned(*PFN_ADD_MSG_HANDLER)(PFN_MESSAGE_HANDLER);
    constexpr auto AddMsgHandler = (PFN_ADD_MSG_HANDLER)0x00524AE0;

    constexpr auto g_MsgHandlers = (PFN_MESSAGE_HANDLER*)0x01B0D5A0;
    constexpr auto g_pcMsgHandlers = (uint32_t*)0x01B0D760;

    constexpr auto g_phWnd = (HWND*)0x01B0D748;
    constexpr auto g_pbIsActive = (uint8_t*)0x01B0D750;
    constexpr auto g_pbClose = (uint8_t*)0x01B0D758;
    constexpr auto g_pbMouseInitialized = (uint8_t*)0x01885461;
    constexpr auto g_pcRedrawServer = (uint32_t*)0x01775698;

    /* Network Game */

    struct BanlistEntry
    {
        char szIp[24];
        BanlistEntry *pNext;
        BanlistEntry *pPrev;
    };

    typedef unsigned(*GetGameType_Type)();
    constexpr auto GetGameType = (GetGameType_Type)0x00470770;

    typedef unsigned(*GetPlayersCount_Type)();
    constexpr auto GetPlayersCount = (GetPlayersCount_Type)0x00484830;

    typedef uint8_t(*GetTeamScore_Type)();
    constexpr auto CtfGetRedScore = (GetTeamScore_Type)0x00475020;
    constexpr auto CtfGetBlueScore = (GetTeamScore_Type)0x00475030;
    constexpr auto TdmGetRedScore = (GetTeamScore_Type)0x004828F0;
    constexpr auto TdmGetBlueScore = (GetTeamScore_Type)0x00482900;
    constexpr auto CtfGetRedFlagPlayer = (CPlayer*(*)())0x00474E60;
    constexpr auto CtfGetBlueFlagPlayer = (CPlayer*(*)())0x00474E70;

    typedef void(*PacketHandler_Type)(const BYTE *pData, const NwAddr *pAddr);
    constexpr auto HandleNewPlayerPacket = (PacketHandler_Type)0x0047A580;

    typedef const char *(*GetJoinFailedStr_Type)(unsigned Reason);
    constexpr auto GetJoinFailedStr = (GetJoinFailedStr_Type)0x0047BE60;

    typedef void(*KickPlayer_Type)(CPlayer *pPlayer);
    constexpr auto KickPlayer = (KickPlayer_Type)0x0047BF00;

    typedef void(*BanIp_Type)(const NwAddr *pAddr);
    constexpr auto BanIp = (BanIp_Type)0x0046D0F0;

    constexpr auto MpResetNetGame = (void(*)())0x0046E450;

    constexpr auto g_pServAddr = (NwAddr*)0x0064EC5C;
    constexpr auto g_pstrServName = (CString*)0x0064EC28;
    constexpr auto g_pbNetworkGame = (uint8_t*)0x0064ECB9;
    constexpr auto g_pbLocalNetworkGame = (uint8_t*)0x0064ECBA;
    constexpr auto g_pbDedicatedServer = (uint32_t*)0x01B0D75C;
    constexpr auto g_pGameOptions = (uint32_t*)0x0064EC40;
    constexpr auto g_ppBanlistFirstEntry = (BanlistEntry**)0x0064EC20;
    constexpr auto g_ppBanlistLastEntry = (BanlistEntry**)0x0064EC24;
    constexpr auto g_pBanlistNullEntry = (BanlistEntry*)0x0064EC08;

    /* Input */
    constexpr auto MouseGetPos = (int(*)(int *pX, int *pY, int *pZ))0x0051E450;
    constexpr auto MouseWasButtonPressed = (int(*)(int BtnIdx))0x0051E5D0;
    constexpr auto KeyGetFromFifo = (int(*)())0x0051F000;

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

    constexpr auto SwitchMenu = (int(*)(int MenuId, bool bForce))0x00434190;
    constexpr auto MenuMainProcessMouse = (void(*)())0x00443D90;
    constexpr auto MenuMainRender = (void(*)())0x004435F0;
    constexpr auto MenuUpdate = (int(*)())0x00434230;
    constexpr auto GetCurrentMenuId = (MenuId(*)())0x00434200;

    constexpr auto g_MenuLevels = (int*)0x00630064;
    constexpr auto g_pCurrentMenuLevel = (int*)0x005967A4;

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

    constexpr auto g_pszRootPath = (char*)0x018060E8;
    constexpr auto g_fFps = (float*)0x005A4018;
    constexpr auto g_pfFramerate = (float*)0x005A4014;
    constexpr auto g_pfMinFramerate = (float*)0x005A4024;
    constexpr auto g_pSimultaneousPing = (uint32_t*)0x00599CD8;
    constexpr auto g_pstrLevelName = (CString*)0x00645FDC;
    constexpr auto g_pstrLevelFilename = (CString*)0x00645FE4;
    constexpr auto g_pstrLevelAuthor = (CString*)0x00645FEC;
    constexpr auto g_pstrLevelDate = (CString*)0x00645FF4;
    constexpr auto g_pBigFontId = (int*)0x006C74C0;
    constexpr auto g_pLargeFontId = (int*)0x0063C05C;
    constexpr auto g_pMediumFontId = (int*)0x0063C060;
    constexpr auto g_pSmallFontId = (int*)0x0063C068;
    constexpr auto g_pbDirectInputDisabled = (bool*)0x005A4F88;
    constexpr auto g_bScoreboardRendered = (bool*)0x006A1448;

    constexpr auto g_pbDbgNetwork = (uint32_t*)0x006FED24;
    constexpr auto g_pbDbgFlagsArray = (uint8_t*)0x0062FE19;
    constexpr auto g_pbRenderEventIcons = (uint8_t*)0x00856500;
    constexpr auto g_pbDbgWeapon = (uint8_t*)0x007CAB59;

    constexpr auto RfBeep = (void(*)(unsigned u1, unsigned u2, unsigned u3, float fVolume))0x00505560;
    constexpr auto InitGame = (void(*)())0x004B13F0;
    constexpr auto CleanupGame = (void(*)())0x004B2D40;
    constexpr auto RunGame = (bool(*)())0x004B2D90;
    constexpr auto GetFileExt = (char *(*)(char *pszPath))0x005143F0;
    constexpr auto SplitScreenStart = (void(*)())0x00480D30;
    constexpr auto GetVersionStr = (void(*)(const char **ppszVersion, const char **a2))0x004B33F0;
    constexpr auto SetNextLevelFilename = (void(*)(CString strFilename, CString strSecond))0x0045E2E0;
    constexpr auto DemoLoadLevel = (void(*)(const char *pszLevelFileName))0x004CC270;
    constexpr auto RenderHitScreen = (void(*)(CPlayer *pPlayer))0x004163C0;
    constexpr auto RenderReticle = (void(*)(CPlayer *pPlayer))0x0043A2C0;
    constexpr auto SetCursorVisible = (void(*)(char bVisible))0x0051E680;
    constexpr auto RflLoad = (int(*)(CString *pstrLevelFilename, CString *a2, char *pszError))0x0045C540;
    constexpr auto DrawScoreboard = (void(*)(bool bDraw))0x00470860;
    constexpr auto DrawScoreboardInternal = (void(*)(bool bDraw))0x00470880;


    /* Strings Table */
    constexpr auto g_ppszStringsTable = (char**)0x007CBBF0;
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
    constexpr auto RfFopen = (RfFopen_Type)0x00574FFE;

    typedef void(*RfSeek_Type)(FILE *pFile, uint32_t Offset, uint32_t Direction);
    constexpr auto RfSeek = (RfSeek_Type)0x00574F14;

    typedef void(*RfDelete_Type)(void *pMem);
    constexpr auto RfDelete = (RfDelete_Type)0x0057360E;

    typedef void *(*RfMalloc_Type)(uint32_t cbSize);
    constexpr auto RfMalloc = (RfMalloc_Type)0x00573B37;
}

#pragma pack(pop)
