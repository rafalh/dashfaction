#pragma once

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdarg>

#include <windef.h>

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
    struct Player;
    struct EntityObj;
    struct AnimMesh;

    /* Math */

    struct Vector3
    {
        float x, y, z;

        Vector3() {}

        Vector3(float x, float y, float z) :
            x(x), y(y), z(z) {}

        Vector3& operator+=(const Vector3& other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3& operator+=(float scalar)
        {
            x += scalar;
            y += scalar;
            z += scalar;
            return *this;
        }

        Vector3& operator*=(float m)
        {
            x *= m;
            y *= m;
            z *= m;
            return *this;
        }

        Vector3& operator/=(float m)
        {
            *this *= 1.0f / m;
        }

        Vector3 operator-() const
        {
            return Vector3(-x, -y, -z);
        }

        Vector3& operator-=(const Vector3& other)
        {
            return (*this += -other);
        }

        Vector3& operator-=(float scalar)
        {
            return (*this += -scalar);
        }

        Vector3 operator+(const Vector3& other) const
        {
            Vector3 tmp = *this;
            tmp += other;
            return tmp;
        }

        Vector3 operator-(const Vector3& other) const
        {
            Vector3 tmp = *this;
            tmp -= other;
            return tmp;
        }

        Vector3 operator+(float scalar) const
        {
            Vector3 tmp = *this;
            tmp += scalar;
            return tmp;
        }

        Vector3 operator-(float scalar) const
        {
            Vector3 tmp = *this;
            tmp -= scalar;
            return tmp;
        }

        Vector3 operator*(float m) const
        {
            return Vector3(x * m, y * m, z * m);
        }

        float len() const
        {
            return sqrtf(lenPow2());
        }

        float lenPow2() const
        {
            return x * x + y * y + z * z;
        }

        void normalize()
        {
            *this /= len();
        }
    };

    struct Matrix3
    {
        Vector3 rows[3];
    };

    /* String */

    static const auto StringAlloc = (char*(*)(unsigned size))0x004FF300;
    static const auto StringFree = (void(*)(void* ptr))0x004FF3A0;

    class String
    {
    public:
        // GCC follows closely Itanium ABI which requires to always pass objects by reference if class has
        // non-trivial destructor. Therefore for passing String by value Pod struct should be used.
        struct Pod
        {
            int32_t buf_size;
            char* buf;
        };

    private:
        Pod m_pod;

    public:
        String()
        {
            const auto fun_ptr = (String&(__thiscall *)(String& this_))0x004FF3B0;
            fun_ptr(*this);
        }

        String(const char* c_str)
        {
            const auto fun_ptr = (String&(__thiscall *)(String& this_, const char* c_str))0x004FF3D0;
            fun_ptr(*this, c_str);
        }

        String(const String& str)
        {
            // Use Pod cast operator to make a clone
            m_pod = str;
        }

        ~String()
        {
            const auto fun_ptr = (void(__thiscall *)(String& this_))0x004FF470;
            fun_ptr(*this);
        }

        operator const char*() const
        {
            return CStr();
        }

        operator Pod() const
        {
            // Make a copy
            const auto fun_ptr = (Pod&(__thiscall *)(Pod& this_, const Pod& str))0x004FF410;
            Pod pod_copy;
            fun_ptr(pod_copy, m_pod);
            return pod_copy;
        }

        String& operator=(const String& other)
        {
            const auto fun_ptr = (String&(__thiscall *)(String& this_, const String& str))0x004FFA20;
            fun_ptr(*this, other);
            return *this;
        }

        const char *CStr() const
        {
            const auto fun_ptr = (const char*(__thiscall *)(const String& this_))0x004FF480;
            return fun_ptr(*this);
        }

        int Size() const
        {
            const auto fun_ptr = (int(__thiscall *)(const String& this_))0x004FF490;
            return fun_ptr(*this);
        }

        static String Format(const char* fmt, ...)
        {
            String str;
            va_list args;
            va_start(args, fmt);
            int n = vsnprintf(nullptr, 0, fmt, args);
            va_end(args);
            va_start(args, fmt);
            str.m_pod.buf_size = n + 1;
            str.m_pod.buf = StringAlloc(str.m_pod.buf_size);
            vsnprintf(str.m_pod.buf, n + 1, fmt, args);
            va_end(args);
            return str;
        }
    };
    static_assert(sizeof(String) == 8);

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

    typedef void(*DcCmdHandler)();

    struct DcCommand
    {
        const char *pszCmd;
        const char *pszDescr;
        void(*pfnHandler)();

        inline static const auto Init = (void(__thiscall *)(rf::DcCommand *This, const char *pszCmd, const char *pszDescr, DcCmdHandler pfnHandler))0x00509A70;
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
        DC_ARG_ANY = 0xFFFFFFFF,
    };

    static const auto DcPrint = (void(*)(const char *pszText, const int *pColor))0x00509EC0;
    static const auto DcPrintf = (void(*)(const char *pszFormat, ...))0x0050B9F0;
    static const auto DcGetArg = (void(*)(int Type, bool bPreserveCase))0x0050AED0;
    static const auto DcRunCmd = (int(*)(const char *pszCmd))0x00509B00;

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

    #define DC_REGISTER_CMD(name, help, handler) \
        do { \
            static rf::DcCommand Dc_##name; \
            rf::DcCommand::Init(&Dc_##name, #name, help, handler); \
        } while (false)

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

    static const auto BmLoad = (int(*)(const char *pszFilename, int a2, bool a3))0x0050F6A0;
    static const auto BmCreateUserBmap = (int(*)(BmPixelFormat PixelFormat, int Width, int Height))0x005119C0;
    static const auto BmConvertFormat = (void(*)(void *pDstBits, BmPixelFormat DstPixelFmt, const void *pSrcBits, BmPixelFormat SrcPixelFmt, int NumPixels))0x0055DD20;
    static const auto BmGetBitmapSize = (void(*)(int BmHandle, int *pWidth, int *pHeight))0x00510630;
    static const auto BmGetFilename = (const char*(*)(int BmHandle))0x00511710;
    static const auto BmLock = (BmPixelFormat(*)(int BmHandle, uint8_t **ppData, uint8_t **ppPalette))0x00510780;
    static const auto BmUnlock = (void(*)(int BmHandle))0x00511700;

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
        Vector3 v3dPos;
        Vector3 vScreenPos;
        int ClipFlags;
        float u0;
        float v0;
        float u1;
        float v1;
        int Color;
    };

    struct GrLockData
    {
        int BmHandle;
        int SectionIdx;
        BmPixelFormat PixelFormat;
        uint8_t *pBits;
        int Width;
        int Height;
        int Pitch;
        int field_1C;
    };
    static_assert(sizeof(GrLockData) == 0x20);

    enum GrTextAlignment
    {
        GR_ALIGN_LEFT = 0,
        GR_ALIGN_CENTER = 1,
        GR_ALIGN_RIGHT = 2,
    };

    static auto &g_pGrDevice = *(IDirect3DDevice8**)0x01CFCBE4;
    static auto &g_GrScreen = *(GrScreen*)0x017C7BC0;

    static auto &g_GrLineMaterial = *(uint32_t*)0x01775B00;
    static auto &g_GrRectMaterial = *(uint32_t*)0x17756C0;
    static auto &g_GrTextMaterial = *(uint32_t*)0x17C7C5C;
    static auto &g_GrBitmapMaterial = *(uint32_t*)0x017756BC;
    static auto &g_GrImageMaterial = *(uint32_t*)0x017756DC;

    static const auto GrGetMaxWidth = (int(*)())0x0050C640;
    static const auto GrGetMaxHeight = (int(*)())0x0050C650;
    static const auto GrGetViewportWidth = (unsigned(*)())0x0050CDB0;
    static const auto GrGetViewportHeight = (unsigned(*)())0x0050CDC0;
    static const auto GrSetColor = (void(*)(unsigned r, unsigned g, unsigned b, unsigned a))0x0050CF80;
    static const auto GrSetColorPtr = (void(*)(uint32_t *pColor))0x0050D000;
    static const auto GrReadBackBuffer = (int(*)(int x, int y, int Width, int Height, void *pBuffer))0x0050DFF0;
    static const auto GrFlushBuffers = (void(*)())0x00559D90;

    static const auto GrDrawRect = (void(*)(unsigned x, unsigned y, unsigned cx, unsigned cy, unsigned Material))0x0050DBE0;
    static const auto GrDrawImage = (void(*)(int BmHandle, int x, int y, int Material))0x0050D2A0;
    static const auto GrDrawBitmapStretched = (void(*)(int BmHandle, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, float a10, float a11, int Material))0x0050D250;
    static const auto GrDrawText = (void(*)(unsigned x, unsigned y, const char *pszText, int Font, unsigned Material))0x0051FEB0;
    static const auto GrDrawAlignedText = (void(*)(GrTextAlignment Align, unsigned x, unsigned y, const char *pszText, int Font, unsigned Material))0x0051FE50;

    static const auto GrRenderLine = (char(*)(const Vector3 *pWorldPos1, const Vector3 *pWorldPos2, int Material))0x00515960;
    static const auto GrRenderSphere = (char(*)(const Vector3 *pvPos, float fRadius, int Material))0x00515CD0;

    static const auto GrFitText = (String* (*)(String* pstrDest, String::Pod Str, int cxMax))0x00471EC0;
    static const auto GrLoadFont = (int(*)(const char *pszFileName, int a2))0x0051F6E0;
    static const auto GrGetFontHeight = (unsigned(*)(int FontId))0x0051F4D0;
    static const auto GrGetTextWidth = (void(*)(int *pOutWidth, int *pOutHeight, const char *pszText, int TextLen, int FontId))0x0051F530;

    static const auto GrLock = (char(*)(int BmHandle, int SectionIdx, GrLockData *pData, int a4))0x0050E2E0;
    static const auto GrUnlock = (void(*)(GrLockData *pData))0x0050E310;

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

    static const auto UiMsgBox = (void(*)(const char *pszTitle, const char *pszText, void(*pfnCallback)(void), bool bInput))0x004560B0;
    static const auto UiCreateDialog = (void(*)(const char *pszTitle, const char *pszText, unsigned cButtons, const char **ppszBtnTitles, void **ppfnCallbacks, unsigned Unknown1, unsigned Unknown2))0x004562A0;
    static const auto UiGetElementFromPos = (int(*)(int x, int y, UiPanel **ppGuiList, signed int cGuiList))0x00442ED0;

    /* Chat */

    enum class ChatMsgColor
    {
        red_white = 0,
        blue_white = 1,
        red_red = 2,
        blue_blue = 3,
        white_white = 4,
        default_ = 5,
    };

    typedef void(*ChatPrint_Type)(String::Pod strText, ChatMsgColor Color, String::Pod Prefix);
    static const auto ChatPrint = (ChatPrint_Type)0x004785A0;

    /* File System */

    typedef int(*PackfileLoad_Type)(const char *pszFileName, const char *pszDir);
    static const auto PackfileLoad = (PackfileLoad_Type)0x0052C070;

    static const auto FsAddDirectoryEx = (int(*)(const char *pszDir, const char *pszExtList, bool bUnknown))0x00514070;

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

    typedef void(*NwSendNotReliablePacket_Type)(const void *pAddr, const void *pPacket, unsigned cbPacket);
    static const auto NwSendNotReliablePacket = (NwSendNotReliablePacket_Type)0x0052A080;

    static const auto NwSendReliablePacket = (void(*)(Player *pPlayer, const uint8_t *pData, unsigned int cbData, int a4))0x00479480;

    typedef void(*NwAddrToStr_Type)(char *pszDest, int cbDest, NwAddr& Addr);
    static const auto NwAddrToStr = (NwAddrToStr_Type)0x00529FE0;

    static const auto NwGetPlayerFromAddr = (Player *(*)(const NwAddr& Addr))0x00484850;
    static const auto NwCompareAddr = (int(*)(const NwAddr &pAddr1, const NwAddr &pAddr2, bool bCheckPort))0x0052A930;

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
        Player *pPlayer;
        CameraType Type;
    };

    static const auto CameraSetFirstPerson = (void(*)(Camera *pCamera))0x0040DDF0;
    static const auto CameraSetFreelook = (void(*)(Camera *pCamera))0x0040DCF0;

    /* Config */

    struct ControlConfigItem
    {
        int16_t DefaultScanCodes[2];
        int16_t DefaultMouseBtnId;
        int16_t field_6;
        int field_8;
        String strName;
        int16_t ScanCodes[2];
        int16_t MouseBtnId;
        int16_t field_1A;
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

    enum GameCtrl
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

    struct PlayerStats
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
        Vector3 field_FC4;
        Matrix3 field_FD0;
        Matrix3 matRotFpsWeapon;
        Vector3 vPosFpsWeapon;
        int field_1024;
        float field_1028;
        int field_102C;
        float field_1030;
        int Pivot1PropId;
        Timer field_1038;
        int field_103C;
        int field_1040;
        int RemoteChargeVisible;
        Vector3 field_1048;
        Matrix3 field_1054;
        int field_1078;
        Timer field_107C;
    };
    static_assert(sizeof(PlayerWeaponInfo) == 0x100, "invalid size");

    struct Player_1094
    {
        Vector3 field_1094;
        Matrix3 field_10A0;
    };

    struct Player
    {
        Player *pNext;
        Player *pPrev;
        String strName;
        PlayerFlags Flags;
        int hEntity;
        int EntityClsId;
        Vector3 field_1C;
        int field_28;
        PlayerStats *pStats;
        char bBlueTeam;
        char bCollide;
        char field_32;
        char field_33;
        AnimMesh *pFpgunMesh;
        AnimMesh *pLastFpgunMesh;
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
        void(__cdecl *field_11DC)(Player *);
        float field_11E0[4];
        int field_11F0;
        float field_11F4;
        int field_11F8;
        int field_11FC;
        CPlayerNetData *pNwData;
    };
    static_assert(sizeof(Player) == 0x1204, "invalid size");

    static auto &g_pPlayersList = *(Player**)0x007C75CC;
    static auto &g_pLocalPlayer = *(Player**)0x007C75D4;

    static const auto KillLocalPlayer = (void(*)())0x004757A0;
    static const auto IsEntityCtrlActive = (char(*)(ControlConfig *pCtrlConf, GameCtrl CtrlId, bool *pWasPressed))0x0043D4F0;
    static const auto GetPlayerFromEntityHandle = (Player*(*)(int32_t hEntity))0x004A3740;

    typedef bool(*IsPlayerEntityInvalid_Type)(Player *pPlayer);
    static const auto IsPlayerEntityInvalid = (IsPlayerEntityInvalid_Type)0x004A4920;

    typedef bool(*IsPlayerDying_Type)(Player *pPlayer);
    static const auto IsPlayerDying = (IsPlayerDying_Type)0x004A4940;

    /* Object */

    enum ObjectType
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

    struct PosRotUnk
    {
        int field_88;
        float field_8C;
        int field_90;
        int field_94;
        float fMass;
        Matrix3 field_9C;
        Matrix3 field_C0;
        Vector3 Pos;
        Vector3 vNewPos;
        Matrix3 matYawRot;
    };

    struct WaterSplashUnk
    {
        Vector3 field_1B4;
        Vector3 field_1C0;
        float field_1CC;
        int field_1D0;
        float field_1D4;
        Vector3 field_1D8;
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
        Matrix3 field_9C;
        Matrix3 field_C0;
        Vector3 vPos;
        Vector3 vNewPos;
        Matrix3 matYawRot;
        Matrix3 field_120;
        Vector3 vVel;
        Vector3 vRotChangeUnk;
        Vector3 field_15C;
        Vector3 field_168;
        Vector3 RotChangeUnkDelta;
        float Radius;
        DynamicArray field_184;
        Vector3 AabbMin;
        Vector3 AabbMax;
        PhysicsFlags Flags;
        int FlagsSplash_1AC;
        float field_1B0;
        WaterSplashUnk WaterSplashUnk;
    };
    static_assert(sizeof(PhysicsInfo) == 0x170, "invalid size");

    struct Object
    {
        void *pRoom;
        Vector3 vLastPosInRoom;
        Object *pNextObj;
        Object *pPrevObj;
        String strName;
        int Uid;
        ObjectType Type;
        int Team;
        int Handle;
        int hOwnerEntityUnk;
        float fLife;
        float fArmor;
        Vector3 vPos;
        Matrix3 matOrient;
        Vector3 vLastPos;
        float fRadius;
        ObjectFlags Flags;
        AnimMesh *pAnimMesh;
        int field_84;
        PhysicsInfo PhysInfo;
        int Friendliness;
        int iMaterial;
        int hParent;
        int UnkPropId_204;
        Vector3 field_208;
        Matrix3 mat214;
        Vector3 vPos3;
        Matrix3 matRot2;
        int *pEmitterListHead;
        int field_26C;
        char KillerId;
        char _pad[3]; // FIXME
        int MultiHandle;
        int pAnim;
        int field_27C;
        Vector3 field_280;
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
        String field_29C;
        int field_2A4;
        int field_2A8;
        int field_2AC;
        char field_2B0[12];
        int field_2BC;
        int field_2C0;
    };

    struct TriggerObj
    {
        Object _Super;
        TriggerObj *pNext;
        TriggerObj *pPrev;
        int Shape;
        Timer ResetsAfterTimer;
        int ResetsAfterMs;
        int ResetsCounter;
        int ResetsTimes;
        float fLastActivationTime;
        int KeyItemClsId;
        int Flags;
        String field_2B4;
        int field_2BC;
        int AirlockRoomUid;
        int ActivatedBy;
        Vector3 BoxSize;
        DynamicArray Links;
        Timer ActivationFailedTimer;
        int hActivationFailedEntity;
        Timer field_2E8;
        char bOneWay;
        char _padding[3];
        float ButtonActiveTimeSeconds;
        Timer field_2F4;
        float fInsideTimeSeconds;
        Timer InsideTimer;
        int AttachedToUid;
        int UseClutterUid;
        char Team;
        char _padding2[3];
    };
    static_assert(sizeof(TriggerObj) == 0x30C, "invalid size");

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
        Vector3 vRotChange;
        Vector3 vPosChange;
        int field_18;
        float field_1C;
        float field_20;
    };

    struct EntityWeapon_2E8_InnerUnk
    {
        Vector3 field_0;
        Vector3 field_C;
        int field_18;
        int field_1C;
        float field_20;
        int field_24;
        DynamicArray field_28;
        char field_34;
        char field_35;
        int16_t field_36;
        int field_38;
        int field_3C;
        int field_40;
        int field_44;
        Matrix3 field_48;
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
        Vector3 field_138;
        int field_144;
        int field_148;
        Vector3 field_14C;
        int field_158;
        int field_15C;
        int field_160;
        Vector3 field_164;
        int field_170;
        Vector3 field_174;
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
        Vector3 field_240;
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
        int16_t field_29E;
        Timer Timer_2A0;
        Timer Timer_2A4;
        Timer Timer_2A8;
        int field_2AC[5];
        int UnkObjHandle;
        Vector3 field_2C4;
        Timer field_2D0;
        int field_2D4;
        int field_2D8;
        Timer field_2DC;
        Timer field_2E0;
        int field_2E4;
        EntityWeapon_2E8 field_2E8;
        SEntityMotion MotionChange;
        Timer field_48C;
        Vector3 field_490;
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
        Vector3 field_4EC;
        Timer Timer_4F8;
        Timer Timer_4FC;
        Vector3 field_500;
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
        Vector3 vRotYaw;
        Vector3 vRotYawDelta;
        Vector3 vRotPitch;
        Vector3 vRotPitchDelta;
        Vector3 field_894;
        Vector3 field_8A0;
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
        Vector3 vViewPos;
        Matrix3 matViewOrient;
        int UnkAmbientSound;
        int field_808;
        int MoveSnd;
        EntityFlags EntityFlags;
        EntityPowerups Powerups;
        Timer UnkTimer818;
        int LaunchSnd;
        int field_820;
        int16_t KillType;
        int16_t field_826;
        int field_828;
        int field_82C;
        Timer field_830;
        int field_834[5];
        AnimMesh *field_848;
        AnimMesh *field_84C;
        int pNanoShield;
        int Snd854;
        MoveModeTbl *MovementMode;
        Matrix3 *field_85C;
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
        Vector3 field_13B8;
        int field_13C4[2];
        Timer field_13CC;
        int field_13D0[2];
        int field_13D8;
        int field_13DC;
        Vector3 field_13E0;
        int field_13EC;
        int field_13F0;
        int UnkBoneId13F4;
        void *field_13F8;
        Timer Timer13FC;
        int SpeakerSound;
        int field_1404;
        float fUnkCountDownTime1408;
        Timer UnkTimer140C;
        AnimMesh *field_1410;
        Timer SplashInCounter;
        DynamicArray field_1418;
        int field_1424;
        int field_1428;
        int field_142C;
        Player *pLocalPlayer;
        Vector3 vPitchMin;
        Vector3 vPitchMax;
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
        AnimMesh *RespawnVfx;
        Timer field_1490;
    };
    static_assert(sizeof(EntityObj) == 0x1494, "invalid size");

    typedef EntityObj *(*EntityGetFromHandle_Type)(uint32_t hEntity);
    static const auto EntityGetFromHandle = (EntityGetFromHandle_Type)0x00426FC0;

    /* Weapons */

    struct WeaponStateAction
    {
        String strName;
        String strAnim;
        int iAnim;
        int SoundId;
        int pSound;
    };

    struct WeaponClass
    {
        String strName;
        String strDisplayName;
        String strV3dFilename;
        int dwV3dType;
        String strEmbeddedV3dFilename;
        int AmmoType;
        String str3rdPersonV3d;
        AnimMesh *p3rdPersonMeshNotSure;
        int Muzzle1PropId;
        int ThirdPersonGrip1MeshProp;
        int dw3rdPersonMuzzleFlashGlare;
        String str1stPersonMesh;
        int *pMesh;
        Vector3 v1stPersonOffset;
        Vector3 v1stPersonOffsetSS;
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
        Vector3 vRicochetSize;
        float fThrustLifetime;
        int cStates;
        int cActions;
        WeaponStateAction States[3];
        WeaponStateAction Actions[7];
        int field_3B8[35];
        int BurstCount;
        float fBurstDelay;
        int BurstLaunchSound;
        int ImpactVclipsCount;
        int ImpactVclips[3];
        String ImpactVclipNames[3];
        float ImpactVclipsRadiusList[3];
        int DecalTexture;
        Vector3 vDecalSize;
        int GlassDecalTexture;
        Vector3 vGlassDecalSize;
        int FpgunShellEjectPropId;
        Vector3 vShellsEjectedBaseDir;
        float fShellEjectVelocity;
        String strShellsEjectedV3d;
        int ShellsEjectedCustomSoundSet;
        float fPrimaryPauseTimeBeforeEject;
        int FpgunClipEjectPropId;
        String strClipsEjectedV3d;
        float fClipsEjectedDropPauseTime;
        int ClipsEjectedCustomSoundSet;
        float fReloadZeroDrain;
        float fCameraShakeDist;
        float fCameraShakeTime;
        String strSilencerV3d;
        int field_4F0_always0;
        int FpgunSilencerPropId;
        String strSparkVfx;
        int field_500_always0;
        int FpgunThrusterPropId;
        int field_508;
        float fAIAttackRange1;
        float fAIAttackRange2;
        float field_514;
        int FpgunWeaponPropId;
        int field_51C_always1neg;
        int WeaponType;
        String strWeaponIcon;
        int DamageType;
        int CyclePos;
        int PrefPos;
        float fFineAimRegSize;
        float fFineAimRegSizeSS;
        String strTracerEffect;
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

    typedef unsigned(*GetGameType_Type)();
    static const auto GetGameType = (GetGameType_Type)0x00470770;

    typedef unsigned(*GetPlayersCount_Type)();
    static const auto GetPlayersCount = (GetPlayersCount_Type)0x00484830;

    typedef uint8_t(*GetTeamScore_Type)();
    static const auto CtfGetRedScore = (GetTeamScore_Type)0x00475020;
    static const auto CtfGetBlueScore = (GetTeamScore_Type)0x00475030;
    static const auto TdmGetRedScore = (GetTeamScore_Type)0x004828F0;
    static const auto TdmGetBlueScore = (GetTeamScore_Type)0x00482900;
    static const auto CtfGetRedFlagPlayer = (Player*(*)())0x00474E60;
    static const auto CtfGetBlueFlagPlayer = (Player*(*)())0x00474E70;

    typedef void(*KickPlayer_Type)(Player *pPlayer);
    static const auto KickPlayer = (KickPlayer_Type)0x0047BF00;

    typedef void(*BanIp_Type)(const NwAddr& addr);
    static const auto BanIp = (BanIp_Type)0x0046D0F0;

    static const auto MultiSetNextWeapon = (void(*)(int WeaponClsId))0x0047FCA0;

    static auto &g_ServAddr = *(NwAddr*)0x0064EC5C;
    static auto &g_strServName = *(String*)0x0064EC28;
    static auto &g_bNetworkGame = *(uint8_t*)0x0064ECB9;
    static auto &g_bLocalNetworkGame = *(uint8_t*)0x0064ECBA;
    static auto &g_bDedicatedServer = *(uint32_t*)0x01B0D75C;
    static auto &g_GameOptions = *(uint32_t*)0x0064EC40;

    /* Input */
    static const auto MouseGetPos = (int(*)(int &x, int &y, int &z))0x0051E450;
    static const auto MouseWasButtonPressed = (int(*)(int BtnIdx))0x0051E5D0;

    /* Game Sequence */

    enum GameSeqState
    {
        GS_INIT = 0x1,
        GS_MAIN_MENU = 0x2,
        GS_EXTRAS_MENU = 0x3,
        GS_INTRO_VIDEO = 0x4,
        GS_LOADING_LEVEL = 0x5,
        GS_SAVE_GAME_MENU = 0x6,
        GS_LOAD_GAME_MENU = 0x7,
        GS_8 = 0x8,
        GS_9 = 0x9,
        GS_AUTO_SAVE = 0xA,
        GS_IN_GAME = 0xB,
        GS_C = 0xC,
        GS_EXIT_GAME = 0xD,
        GS_OPTIONS_MENU = 0xE,
        GS_MP_MENU = 0xF,
        GS_HELP = 0x10,
        GS_11 = 0x11,
        GS_IN_SPLITSCREEN_GAME = 0x12,
        GS_GAME_OVER = 0x13,
        GS_14 = 0x14,
        GS_GAME_INTRO = 0x15,
        GS_16 = 0x16,
        GS_CREDITS = 0x17,
        GS_BOMB_TIMER = 0x18,
        GS_19 = 0x19,
        GS_1A = 0x1A,
        GS_1B = 0x1B,
        GS_1C = 0x1C,
        GS_1D = 0x1D,
        GS_MP_SERVER_LIST_MENU = 0x1E,
        GS_MP_SPLITSCREEN_MENU = 0x1F,
        GS_MP_CREATE_GAME_MENU = 0x20,
        GS_21 = 0x21,
        GS_MP_LIMBO = 0x22,
    };

    static const auto GameSeqSetState = (int(*)(int state, bool force))0x00434190;
    static const auto GameSeqGetState = (GameSeqState(*)())0x00434200;

    /* Other */

    struct RflLightmap
    {
        uint8_t *pUnk;
        int w;
        int h;
        uint8_t *pBuf;
        int BmHandle;
        int unk2;
    };

    static const auto g_pszRootPath = (char*)0x018060E8;
    static auto &g_fFps = *(float*)0x005A4018;
    static auto &g_fFramerate = *(float*)0x005A4014;
    static auto &g_fMinFramerate = *(float*)0x005A4024;
    static auto &g_strLevelName = *(String*)0x00645FDC;
    static auto &g_strLevelFilename = *(String*)0x00645FE4;
    static auto &g_strLevelAuthor = *(String*)0x00645FEC;
    static auto &g_strLevelDate = *(String*)0x00645FF4;
    static auto &g_BigFontId = *(int*)0x006C74C0;
    static auto &g_LargeFontId = *(int*)0x0063C05C;
    static auto &g_MediumFontId = *(int*)0x0063C060;
    static auto &g_SmallFontId = *(int*)0x0063C068;
    static auto &g_bDirectInputDisabled = *(bool*)0x005A4F88;
    static auto &g_strDefaultPlayerWeapon = *(String*)0x007C7600;
    static auto &g_active_cutscene = *reinterpret_cast<void**>(0x00645320);

    static const auto RfBeep = (void(*)(unsigned u1, unsigned u2, unsigned u3, float fVolume))0x00505560;
    static const auto GetFileExt = (char *(*)(const char *pszPath))0x005143F0;
    static const auto SplitScreenStart = (void(*)())0x00480D30;
    static const auto SetNextLevelFilename = (void(*)(String::Pod strFilename, String::Pod strSecond))0x0045E2E0;
    static const auto DemoLoadLevel = (void(*)(const char *pszLevelFileName))0x004CC270;
    static const auto SetCursorVisible = (void(*)(bool bVisible))0x0051E680;
    static const auto CutsceneIsActive = reinterpret_cast<bool(*)()>(0x0045BE80);
    static const auto Timer__GetTimeLeftMs = reinterpret_cast<int (__thiscall*)(void* timer)>(0x004FA420);

    /* Strings Table */
    namespace strings {
        static const auto array = (char**)0x007CBBF0;
        static const auto &player = array[675];
        static const auto &frags = array[676];
        static const auto &ping = array[677];
        static const auto &caps = array[681];
        static const auto &was_killed_by_his_own_hand = array[693];
        static const auto &was_killed_by = array[694];
        static const auto &was_killed_mysteriously = array[695];
        static const auto &score = array[720];
        static const auto &player_name = array[835];
        static const auto &exiting_game = array[884];
        static const auto &usage = array[886];
        static const auto &you_killed_yourself = array[942];
        static const auto &you_just_got_beat_down_by = array[943];
        static const auto &you_were_killed_by = array[944];
        static const auto &you_killed = array[945];
        static const auto &got_beat_down_by = array[946];
        static const auto &kicking_player = array[958];
    }

    /* RF stdlib functions are not compatible with GCC */

    typedef void(*Free_Type)(void *pMem);
    static const auto Free = (Free_Type)0x00573C71;

    typedef void *(*Malloc_Type)(uint32_t cbSize);
    static const auto Malloc = (Malloc_Type)0x00573B37;
}

#pragma pack(pop)
