#pragma once

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdarg>

#include <windef.h>

typedef BOOL WINBOOL;
#include <d3d8.h>

#include <MemUtils.h>
#include "utils.h"

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
            return *this;
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

    static auto& StringAlloc = AddrAsRef<char*(unsigned size)>(0x004FF300);
    static auto& StringFree = AddrAsRef<void(char* ptr)>(0x004FF3A0);

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
            auto& fun_ptr = AddrAsRef<String& __thiscall(String& self)>(0x004FF3B0);
            fun_ptr(*this);
        }

        String(const char* c_str)
        {
            auto& fun_ptr = AddrAsRef<String& __thiscall(String& self, const char* c_str)>(0x004FF3D0);
            fun_ptr(*this, c_str);
        }

        String(const String& str)
        {
            // Use Pod cast operator to make a clone
            m_pod = str;
        }

        ~String()
        {
            auto& fun_ptr = AddrAsRef<void __thiscall(String& self)>(0x004FF470);
            fun_ptr(*this);
        }

        operator const char*() const
        {
            return CStr();
        }

        operator Pod() const
        {
            // Make a copy
            auto& fun_ptr = AddrAsRef<Pod& __thiscall(Pod& self, const Pod& str)>(0x004FF410);
            Pod pod_copy;
            fun_ptr(pod_copy, m_pod);
            return pod_copy;
        }

        String& operator=(const String& other)
        {
            auto& fun_ptr = AddrAsRef<String& __thiscall(String& self, const String& str)>(0x004FFA20);
            fun_ptr(*this, other);
            return *this;
        }

        const char *CStr() const
        {
            auto& fun_ptr = AddrAsRef<const char* __thiscall(const String& self)>(0x004FF480);
            return fun_ptr(*this);
        }

        int Size() const
        {
            auto& fun_ptr = AddrAsRef<int __thiscall(const String& self)>(0x004FF490);
            return fun_ptr(*this);
        }

        PRINTF_FMT_ATTRIBUTE(1, 2)
        static inline String Format(const char* format, ...)
        {
            String str;
            va_list args;
            va_start(args, format);
            int size = vsnprintf(nullptr, 0, format, args) + 1;
            va_end(args);
            str.m_pod.buf_size = size;
            str.m_pod.buf = StringAlloc(str.m_pod.buf_size);
            va_start(args, format);
            vsnprintf(str.m_pod.buf, size, format, args);
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
        void *Data;
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
        const char *Cmd;
        const char *Descr;
        void(*Func)();

        inline static auto& Init =
            AddrAsRef<void __thiscall(rf::DcCommand *self, const char *cmd, const char *descr, DcCmdHandler handler)>(0x00509A70);
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

    static auto& DcPrint = AddrAsRef<void(const char *text, const int *color)>(0x00509EC0);
    static auto& DcGetArg = AddrAsRef<void(int type, bool preserve_case)>(0x0050AED0);
    static auto& DcRunCmd = AddrAsRef<int(const char *cmd)>(0x00509B00);

    // Note: DcPrintf is reimplemented to allow static validation of format string
    PRINTF_FMT_ATTRIBUTE(1, 2)
    inline void DcPrintf(const char* format, ...)
    {
        String str;
        va_list args;
        va_start(args, format);
        int size = vsnprintf(nullptr, 0, format, args) + 1;
        va_end(args);
        std::unique_ptr<char[]> buf(new char[size]);
        va_start(args, format);
        vsnprintf(buf.get(), size, format, args);
        va_end(args);
        DcPrint(buf.get(), nullptr);
    }

    //static auto& g_DcCommands = AddrAsRef<DcCommand*[30]>(0x01775530);
    static auto& g_DcNumCommands = AddrAsRef<uint32_t>(0x0177567C);

    static auto& g_DcRun = AddrAsRef<uint32_t>(0x01775110);
    static auto& g_DcHelp = AddrAsRef<uint32_t>(0x01775224);
    static auto& g_DcStatus = AddrAsRef<uint32_t>(0x01774D00);
    static auto& g_DcArgType = AddrAsRef<uint32_t>(0x01774D04);
    static auto& g_DcStrArg = AddrAsRef<char[256]>(0x0175462C);
    static auto& g_DcIntArg = AddrAsRef<int>(0x01775220);
    static auto& g_DcFloatArg = AddrAsRef<float>(0x01754628);
    static auto& g_DcCmdLine = AddrAsRef<char[256]>(0x01775330);
    static auto& g_DcCmdLineLen = AddrAsRef<uint32_t>(0x0177568C);

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

    static auto& BmLoad = AddrAsRef<int(const char *filename, int a2, bool a3)>(0x0050F6A0);
    static auto& BmCreateUserBmap = AddrAsRef<int(BmPixelFormat pixel_format, int width, int height)>(0x005119C0);
    static auto& BmConvertFormat = AddrAsRef<void(void *dst_bits, BmPixelFormat dst_pixel_fmt, const void *src_bits, BmPixelFormat src_pixel_fmt, int num_pixels)>(0x0055DD20);
    static auto& BmGetBitmapSize = AddrAsRef<void(int bm_handle, int *width, int *height)>(0x00510630);
    static auto& BmGetFilename = AddrAsRef<const char*(int bm_handle)>(0x00511710);
    static auto& BmLock = AddrAsRef<BmPixelFormat(int bm_handle, uint8_t **pp_data, uint8_t **pp_palette)>(0x00510780);
    static auto& BmUnlock = AddrAsRef<void(int bm_handle)>(0x00511700);

    /* Graphics */

    struct GrScreen
    {
        int Signature;
        int MaxWidth;
        int MaxHeight;
        int Mode;
        int WindowMode;
        int field_14;
        float Aspect;
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
        Vector3 Pos3D;
        Vector3 ScreenPos;
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
        uint8_t *Bits;
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

    static auto& g_GrDevice = AddrAsRef<IDirect3DDevice8*>(0x01CFCBE4);
    static auto& g_GrScreen = AddrAsRef<GrScreen>(0x017C7BC0);

    static auto& g_GrLineMaterial = AddrAsRef<uint32_t>(0x01775B00);
    static auto& g_GrRectMaterial = AddrAsRef<uint32_t>(0x17756C0);
    static auto& g_GrTextMaterial = AddrAsRef<uint32_t>(0x17C7C5C);
    static auto& g_GrBitmapMaterial = AddrAsRef<uint32_t>(0x017756BC);
    static auto& g_GrImageMaterial = AddrAsRef<uint32_t>(0x017756DC);

    static auto& GrGetMaxWidth = AddrAsRef<int()>(0x0050C640);
    static auto& GrGetMaxHeight = AddrAsRef<int()>(0x0050C650);
    static auto& GrGetViewportWidth = AddrAsRef<unsigned()>(0x0050CDB0);
    static auto& GrGetViewportHeight = AddrAsRef<unsigned()>(0x0050CDC0);
    static auto& GrSetColor = AddrAsRef<void(unsigned r, unsigned g, unsigned b, unsigned a)>(0x0050CF80);
    static auto& GrSetColorPtr = AddrAsRef<void(uint32_t *color)>(0x0050D000);
    static auto& GrReadBackBuffer = AddrAsRef<int(int x, int y, int width, int height, void *buffer)>(0x0050DFF0);
    static auto& GrFlushBuffers = AddrAsRef<void()>(0x00559D90);

    static auto& GrDrawRect = AddrAsRef<void(unsigned x, unsigned y, unsigned cx, unsigned cy, unsigned material)>(0x0050DBE0);
    static auto& GrDrawImage = AddrAsRef<void(int bm_handle, int x, int y, int material)>(0x0050D2A0);
    static auto& GrDrawBitmapStretched = AddrAsRef<void(int bm_handle, int dst_x, int dst_y, int dst_w, int dst_h, int src_x, int src_y, int src_w, int src_h, float a10, float a11, int material)>(0x0050D250);
    static auto& GrDrawText = AddrAsRef<void(unsigned x, unsigned y, const char *text, int font, unsigned material)>(0x0051FEB0);
    static auto& GrDrawAlignedText = AddrAsRef<void(GrTextAlignment align, unsigned x, unsigned y, const char *text, int font, unsigned material)>(0x0051FE50);

    static auto& GrRenderLine = AddrAsRef<char(const Vector3 *world_pos1, const Vector3 *world_pos2, int material)>(0x00515960);
    static auto& GrRenderSphere = AddrAsRef<char(const Vector3 *pv_pos, float f_radius, int material)>(0x00515CD0);

    static auto& GrFitText = AddrAsRef<String* (String* pstr_dest, String::Pod str, int cx_max)>(0x00471EC0);
    static auto& GrLoadFont = AddrAsRef<int(const char *file_name, int a2)>(0x0051F6E0);
    static auto& GrGetFontHeight = AddrAsRef<unsigned(int font_id)>(0x0051F4D0);
    static auto& GrGetTextWidth = AddrAsRef<void(int *out_width, int *out_height, const char *text, int text_len, int font_id)>(0x0051F530);

    static auto& GrLock = AddrAsRef<char(int bm_handle, int section_idx, GrLockData *data, int a4)>(0x0050E2E0);
    static auto& GrUnlock = AddrAsRef<void(GrLockData *data)>(0x0050E310);

    /* User Interface (UI) */

    struct UiPanel
    {
        void(**field_0)();
        UiPanel *Parent;
        char field_8;
        char field_9;
        int x;
        int y;
        int w;
        int h;
        int Id;
        void(*OnClick)();
        int field_24;
        int BgTexture;
    };

    static auto& UiMsgBox = AddrAsRef<void(const char *title, const char *text, void(*callback)(), bool input)>(0x004560B0);
    using UiDialogCallbackPtr = void (*)();
    static auto& UiCreateDialog =
        AddrAsRef<void(const char *title, const char *text, unsigned num_buttons, const char **ppsz_btn_titles,
                       UiDialogCallbackPtr *callbacks, unsigned unknown1, unsigned unknown2)>(0x004562A0);
    static auto& UiGetElementFromPos = AddrAsRef<int(int x, int y, UiPanel **ui_widgets, signed int num_ui_widgets)>(0x00442ED0);

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

    static auto& ChatPrint = AddrAsRef<void(String::Pod str_text, ChatMsgColor color, String::Pod prefix)>(0x004785A0);

    /* File System */

    static auto& PackfileLoad = AddrAsRef<int(const char *file_name, const char *dir)>(0x0052C070);
    static auto& FsAddDirectoryEx = AddrAsRef<int(const char *dir, const char *ext_list, bool unknown)>(0x00514070);

    /* Network */

    struct NwAddr
    {
        uint32_t IpAddr;
        uint16_t Port;
        uint16_t _unused;

        bool operator==(const NwAddr &other) const
        {
            return IpAddr == other.IpAddr && Port == other.Port;
        }

        bool operator!=(const NwAddr &other) const
        {
            return !(*this == other);
        }
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

    struct PlayerNetData
    {
        NwAddr Addr;
        NwPlayerFlags Flags;
        int field_C;
        int ConnectionId;
        uint8_t PlayerId;
        uint8_t _unused[3];
        int field_18;
        NwStats Stats;
        int Ping;
        float field_5B0;
        char PacketBuf[512];
        int cbPacketBuf;
        char OutReliableBuf[512];
        int cbOutReliableBuf;
        int ConnectionSpeed;
        int field_9C0;
        Timer field_9C4;
    };
    static_assert(sizeof(PlayerNetData) == 0x9C8, "invalid size");

    static auto& NwSendNotReliablePacket =
        AddrAsRef<void(const void *addr, const void *packet, unsigned cb_packet)>(0x0052A080);
    static auto& NwSendReliablePacket =
        AddrAsRef<void(Player *player, const uint8_t *data, unsigned int num_bytes, int a4)>(0x00479480);
    static auto& NwAddrToStr = AddrAsRef<void(char *dest, int cb_dest, NwAddr& addr)>(0x00529FE0);
    static auto& NwGetPlayerFromAddr = AddrAsRef<Player*(const NwAddr& addr)>(0x00484850);
    static auto& NwCompareAddr = AddrAsRef<int(const NwAddr &addr1, const NwAddr &addr2, bool check_port)>(0x0052A930);

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
        EntityObj *CameraEntity;
        Player *Player;
        CameraType Type;
    };

    static auto& CameraSetFirstPerson = AddrAsRef<void(Camera *camera)>(0x0040DDF0);
    static auto& CameraSetFreelook = AddrAsRef<void(Camera *camera)>(0x0040DCF0);

    /* Config */

    struct ControlConfigItem
    {
        int16_t DefaultScanCodes[2];
        int16_t DefaultMouseBtnId;
        int16_t field_6;
        int field_8;
        String Name;
        int16_t ScanCodes[2];
        int16_t MouseBtnId;
        int16_t field_1A;
    };

    struct ControlConfig
    {
        float MouseSensitivity;
        int MouseLook;
        int field_EC;
        ControlConfigItem Keys[128];
        int CtrlCount;
        int field_EF4;
        int field_EF8;
        int field_EFC;
        int field_F00;
        char field_F04;
        char MouseYInvert;
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
        char FirstPersonWeaponVisible;
        char field_E53;
        char CrouchStay;
        char field_F39;
        char field_F3A;
        char HudVisible;
        char field_F3C;
        char field_E59;
        char field_E5A;
        char field_E5B;
        char AutoswitchWeapons;
        char AutoswitchExplosives;
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
        char Name[12];
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
        int16_t score;
        int16_t caps;
    };

    struct PlayerWeaponInfo
    {
        int NextWeaponClsId;
        Timer WeaponSwitchTimer;
        Timer UnkReloadTimer;
        int field_F8C;
        Timer UnkTimerF90;
        char InScopeView;
        char field_F95;
        char field_F96;
        char field_F97;
        float ScopeZoom;
        char field_F9C;
        char field_1D;
        char field_1E;
        char field_1F;
        int field_FA0;
        Timer field_FA4;
        Timer TimerFA8;
        Timer field_FAC;
        char RailgunScanner;
        char ScannerView;
        Color clrUnkR;
        char field_FB6;
        char field_FB7;
        int field_FB8;
        int field_FBC;
        float field_FC0;
        Vector3 field_FC4;
        Matrix3 field_FD0;
        Matrix3 RotFpsWeapon;
        Vector3 PosFpsWeapon;
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

    struct Player1094
    {
        Vector3 field_1094;
        Matrix3 field_10A0;
    };

    struct Player
    {
        Player *Next;
        Player *Prev;
        String Name;
        PlayerFlags Flags;
        int Entity_handle;
        int EntityClsId;
        Vector3 field_1C;
        int field_28;
        PlayerStats *Stats;
        char BlueTeam;
        char Collide;
        char field_32;
        char field_33;
        AnimMesh *FpgunMesh;
        AnimMesh *LastFpgunMesh;
        Timer Timer3C;
        int FpgunMuzzleProps[2];
        int FpgunAmmoDigit1Prop;
        int FpgunAmmoDigit2Prop;
        int field_50[24];
        char field_B0;
        char IsCrouched;
        char field_B2;
        char field_B3;
        int ViewObj_handle;
        Timer WeaponSwitchTimer2;
        Timer UseKeyTimer;
        Timer field_C0;
        Camera *Camera;
        int xViewport;
        int yViewport;
        int cxViewport;
        int cyViewport;
        float Fov;
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
        Player1094 field_1094;
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
        PlayerNetData *NwData;
    };
    static_assert(sizeof(Player) == 0x1204, "invalid size");

    static auto& g_PlayerList = AddrAsRef<Player*>(0x007C75CC);
    static auto& g_LocalPlayer = AddrAsRef<Player*>(0x007C75D4);

    static auto& KillLocalPlayer = AddrAsRef<void()>(0x004757A0);
    static auto& IsEntityCtrlActive =
        AddrAsRef<char(ControlConfig *ctrl_conf, GameCtrl ctrl_id, bool *was_pressed)>(0x0043D4F0);
    static auto& GetPlayerFromEntityHandle = AddrAsRef<Player*(int32_t entity_handle)>(0x004A3740);
    static auto& IsPlayerEntityInvalid = AddrAsRef<bool(Player *player)>(0x004A4920);
    static auto& IsPlayerDying = AddrAsRef<bool(Player *player)>(0x004A4940);

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
        float Mass;
        Matrix3 field_9C;
        Matrix3 field_C0;
        Vector3 Pos;
        Vector3 NewPos;
        Matrix3 YawRot;
    };

    struct WaterSplashUnk
    {
        Vector3 field_1B4;
        Vector3 field_1C0;
        float field_1CC;
        int field_1D0;
        float field_1D4;
        Vector3 field_1D8;
        int UnkEntity_handle;
        int field_1E8;
        int WaterSplash_1EC;
        int field_3C;
        int field_40;
    };

    struct PhysicsInfo
    {
        float Elasticity;
        float field_8C;
        float Friction;
        int field_94;
        float Mass;
        Matrix3 field_9C;
        Matrix3 field_C0;
        Vector3 Pos;
        Vector3 NewPos;
        Matrix3 YawRot;
        Matrix3 field_120;
        Vector3 Vel;
        Vector3 RotChangeUnk;
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
        void *Room;
        Vector3 LastPosInRoom;
        Object *NextObj;
        Object *PrevObj;
        String Name;
        int Uid;
        ObjectType Type;
        int Team;
        int Handle;
        int OwnerEntityUnk_handle;
        float Life;
        float Armor;
        Vector3 Pos;
        Matrix3 Orient;
        Vector3 LastPos;
        float Radius;
        ObjectFlags Flags;
        AnimMesh *AnimMesh;
        int field_84;
        PhysicsInfo PhysInfo;
        int Friendliness;
        int Material;
        int Parent_handle;
        int UnkPropId_204;
        Vector3 field_208;
        Matrix3 mat214;
        Vector3 Pos3;
        Matrix3 Rot2;
        int *EmitterListHead;
        int field_26C;
        char KillerId;
        char _pad[3]; // FIXME
        int MultiHandle;
        int Anim;
        int field_27C;
        Vector3 field_280;
    };
    static_assert(sizeof(Object) == 0x28C, "invalid size");

    struct EventObj
    {
        int Vtbl;
        Object _Super;
        int EventType;
        float Delay;
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
        ItemObj *Next;
        ItemObj *Prev;
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
        TriggerObj *Next;
        TriggerObj *Prev;
        int Shape;
        Timer ResetsAfterTimer;
        int ResetsAfterMs;
        int ResetsCounter;
        int ResetsTimes;
        float LastActivationTime;
        int KeyItemClsId;
        int Flags;
        String field_2B4;
        int field_2BC;
        int AirlockRoomUid;
        int ActivatedBy;
        Vector3 BoxSize;
        DynamicArray Links;
        Timer ActivationFailedTimer;
        int ActivationFailedEntity_handle;
        Timer field_2E8;
        char OneWay;
        char _padding[3];
        float ButtonActiveTimeSeconds;
        Timer field_2F4;
        float InsideTimeSeconds;
        Timer InsideTimer;
        int AttachedToUid;
        int UseClutterUid;
        char Team;
        char _padding2[3];
    };
    static_assert(sizeof(TriggerObj) == 0x30C, "invalid size");

    static auto& g_ItemObjList = AddrAsRef<ItemObj>(0x00642DD8);

    static auto& ObjGetFromUid = AddrAsRef<Object *(int uid)>(0x0048A4A0);

    /* Entity */

    typedef void EntityClass;

    struct MoveModeTbl
    {
        char Available;
        char unk, unk2, unk3;
        int Id;
        int MoveRefX;
        int MoveRefY;
        int MoveRefZ;
        int RotRefX;
        int RotRefY;
        int RotRefZ;
    };

    struct SWeaponSelection
    {
        EntityClass *EntityCls;
        int WeaponClsId;
        int WeaponClsId2;
    };

    struct SEntityMotion
    {
        Vector3 RotChange;
        Vector3 PosChange;
        int field_18;
        float field_1C;
        float field_20;
    };

    struct EntityWeapon2E8InnerUnk
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

    struct EntityWeapon2E8
    {
        int field_0;
        int field_4;
        int field_8[5];
        EntityWeapon2E8InnerUnk field_1C;
        EntityWeapon2E8InnerUnk field_98;
        int field_114;
        int field_118;
        Timer Timer_11C;
        int field_120;
        int field_124;
        int field_128;
        int field_12C;
        int EntityUnk_handle;
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
        EntityObj *Entity;
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
        float CreateTime;
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
        EntityWeapon2E8 field_2E8;
        SEntityMotion MotionChange;
        Timer field_48C;
        Vector3 field_490;
        float LastDmgTime;
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
        float MovementRadius;
        float field_524_;
        int field_528;
        int field_52C;
        int Flags;
    };
    static_assert(sizeof(EntityWeaponInfo) == 0x534, "invalid size");

    struct EntityCameraInfo
    {
        int field_860;
        Vector3 RotYaw;
        Vector3 RotYawDelta;
        Vector3 RotPitch;
        Vector3 RotPitchDelta;
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
        EntityObj *Next;
        EntityObj *Prev;
        EntityClass *Class;
        int ClassId;
        EntityClass *Class2;
        EntityWeaponInfo WeaponInfo;
        Vector3 ViewPos;
        Matrix3 ViewOrient;
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
        int NanoShield;
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
        float UnkCountDownTime1408;
        Timer UnkTimer140C;
        AnimMesh *field_1410;
        Timer SplashInCounter;
        DynamicArray field_1418;
        int field_1424;
        int field_1428;
        int field_142C;
        Player *LocalPlayer;
        Vector3 PitchMin;
        Vector3 PitchMax;
        int KillerEntity_handle;
        int RiotShieldClutterHandle;
        int field_1454;
        Timer field_1458;
        int UnkClutterHandles[2];
        float Time;
        int field_1468;
        int UnkEntity_handle;
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

    static auto& EntityGetFromHandle = AddrAsRef<EntityObj*(uint32_t handle)>(0x00426FC0);

    /* Weapons */

    struct WeaponStateAction
    {
        String Name;
        String AnimName;
        int AnimId;
        int SoundId;
        int SoundHandle;
    };

    struct WeaponClass
    {
        String Name;
        String DisplayName;
        String V3dFilename;
        int V3dType;
        String EmbeddedV3dFilename;
        int AmmoType;
        String ThirdPersonV3d;
        AnimMesh *ThirdPersonMeshNotSure;
        int Muzzle1PropId;
        int ThirdPersonGrip1MeshProp;
        int ThirdPersonMuzzleFlashGlare;
        String FirstPersonMesh;
        void *Mesh;
        Vector3 FirstPersonOffset;
        Vector3 FirstPersonOffsetSS;
        int FirstPersonMuzzleFlashBitmap;
        float FirstPersonMuzzleFlashRadius;
        int FirstPersonAltMuzzleFlashBitmap;
        float FirstPersonAltMuzzleFlashRadius;
        float FirstPersonFov;
        float SplitscreenFov;
        int NumProjectiles;
        int ClipSizeSP;
        int ClipSizeMP;
        int ClipSize;
        float ClipReloadTime;
        float ClipDrainTime;
        int Bitmap;
        int TrailEmitter;
        float HeadRadius;
        float TailRadius;
        float HeadLen;
        int IsSecondary;
        float CollisionRadius;
        int Lifetime;
        float LifetimeSP;
        float LifetimeMulti;
        float Mass;
        float Velocity;
        float VelocityMulti;
        float VelocitySP;
        float LifetimeMulVel;
        float FireWait;
        float AltFireWait;
        float SpreadDegreesSP;
        float SpreadDegreesMulti;
        int SpreadDegrees;
        float AltSpreadDegreesSP;
        float AltSpreadDegreesMulti;
        int AltSpreadDegrees;
        float AiSpreadDegreesSP;
        float AiSpreadDegreesMulti;
        int AiSpreadDegrees;
        float AiAltSpreadDegreesSP;
        float AiAltSpreadDegreesMulti;
        int AiAltSpreadDegrees;
        float Damage;
        float DamageMulti;
        float AltDamage;
        float AltDamageMulti;
        float AIDmgScaleSP;
        float AIDmgScaleMP;
        int AIDmgScale;
        int field_124;
        int field_128;
        int field_12C;
        float TurnTime;
        float ViewCone;
        float ScanningRange;
        float WakeupTime;
        float DrillTime;
        float DrillRange;
        float DrillCharge;
        float CraterRadius;
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
        float StartDelay;
        int StopSound;
        float DamageRadius;
        float DamageRadiusSP;
        float DamageRadiusMulti;
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
        int MaxAmmoSP;
        int MaxAmmoMP;
        int MaxAmmo;
        int Flags;
        int Flags2;
        int field_26C;
        int field_270;
        int TracerFrequency;
        int field_278;
        float PiercingPower;
        float RicochetAngle;
        int RicochetBitmap;
        Vector3 RicochetSize;
        float ThrustLifetime;
        int NumStates;
        int NumActions;
        WeaponStateAction States[3];
        WeaponStateAction Actions[7];
        int field_3B8[35];
        int BurstCount;
        float BurstDelay;
        int BurstLaunchSound;
        int ImpactVclipsCount;
        int ImpactVclips[3];
        String ImpactVclipNames[3];
        float ImpactVclipsRadiusList[3];
        int DecalTexture;
        Vector3 DecalSize;
        int GlassDecalTexture;
        Vector3 GlassDecalSize;
        int FpgunShellEjectPropId;
        Vector3 ShellsEjectedBaseDir;
        float ShellEjectVelocity;
        String ShellsEjectedV3d;
        int ShellsEjectedCustomSoundSet;
        float PrimaryPauseTimeBeforeEject;
        int FpgunClipEjectPropId;
        String ClipsEjectedV3d;
        float ClipsEjectedDropPauseTime;
        int ClipsEjectedCustomSoundSet;
        float ReloadZeroDrain;
        float CameraShakeDist;
        float CameraShakeTime;
        String SilencerV3d;
        int field_4F0_always0;
        int FpgunSilencerPropId;
        String SparkVfx;
        int field_500_always0;
        int FpgunThrusterPropId;
        int field_508;
        float AIAttackRange1;
        float AIAttackRange2;
        float field_514;
        int FpgunWeaponPropId;
        int field_51C_always1neg;
        int WeaponType;
        String WeaponIcon;
        int DamageType;
        int CyclePos;
        int PrefPos;
        float FineAimRegSize;
        float FineAimRegSizeSS;
        String TracerEffect;
        int field_548_always0;
        float MultiBBoxSizeFactor;
    };
    static_assert(sizeof(WeaponClass) == 0x550, "invalid size");

    static auto& g_WeaponClasses = AddrAsRef<WeaponClass[64]>(0x0085CD08);
    static auto& g_RiotStickClsId = AddrAsRef<int32_t>(0x00872468);
    static auto& g_RemoteChargeClsId = AddrAsRef<int32_t>(0x0087210C);

    /* Window */

    typedef void(*MsgHandlerPtr)(UINT, WPARAM, LPARAM);
    static auto &AddMsgHandler = AddrAsRef<unsigned(MsgHandlerPtr)>(0x00524AE0);

    static auto& g_MsgHandlers = AddrAsRef<MsgHandlerPtr[32]>(0x01B0D5A0);
    static auto& g_NumMsgHandlers = AddrAsRef<uint32_t>(0x01B0D760);

    static auto& g_MainWnd = AddrAsRef<HWND>(0x01B0D748);
    static auto& g_IsActive = AddrAsRef<uint8_t>(0x01B0D750);
    static auto& g_Close = AddrAsRef<uint8_t>(0x01B0D758);
    static auto& g_MouseInitialized = AddrAsRef<uint8_t>(0x01885461);
    static auto& g_NumRedrawServer = AddrAsRef<uint32_t>(0x01775698);

    /* Network Game */

    static auto& GetGameType = AddrAsRef<unsigned()>(0x00470770);
    static auto& GetPlayersCount = AddrAsRef<unsigned()>(0x00484830);
    static auto& CtfGetRedScore = AddrAsRef<uint8_t()>(0x00475020);
    static auto& CtfGetBlueScore = AddrAsRef<uint8_t()>(0x00475030);
    static auto& TdmGetRedScore = AddrAsRef<uint8_t()>(0x004828F0);
    static auto& TdmGetBlueScore = AddrAsRef<uint8_t()>(0x00482900);
    static auto& CtfGetRedFlagPlayer = AddrAsRef<Player*()>(0x00474E60);
    static auto& CtfGetBlueFlagPlayer = AddrAsRef<Player*()>(0x00474E70);
    static auto& KickPlayer = AddrAsRef<void(Player *player)>(0x0047BF00);
    static auto& BanIp = AddrAsRef<void(const NwAddr& addr)>(0x0046D0F0);
    static auto& MultiSetNextWeapon = AddrAsRef<void(int weapon_cls_id)>(0x0047FCA0);

    static auto& g_ServAddr = AddrAsRef<NwAddr>(0x0064EC5C);
    static auto& g_ServName = AddrAsRef<String>(0x0064EC28);
    static auto& g_IsNetworkGame = AddrAsRef<uint8_t>(0x0064ECB9);
    static auto& g_IsLocalNetworkGame = AddrAsRef<uint8_t>(0x0064ECBA);
    static auto& g_IsDedicatedServer = AddrAsRef<uint32_t>(0x01B0D75C);
    static auto& g_GameOptions = AddrAsRef<uint32_t>(0x0064EC40);

    /* Input */
    static auto& MouseGetPos = AddrAsRef<int(int &x, int &y, int &z)>(0x0051E450);
    static auto& MouseWasButtonPressed = AddrAsRef<int(int btn_idx)>(0x0051E5D0);

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

    static auto& GameSeqSetState = AddrAsRef<int(int state, bool force)>(0x00434190);
    static auto& GameSeqGetState = AddrAsRef<GameSeqState()>(0x00434200);

    /* Other */

    struct RflLightmap
    {
        uint8_t *Unk;
        int w;
        int h;
        uint8_t *Buf;
        int BmHandle;
        int unk2;
    };

    static auto& g_RootPath = AddrAsRef<char[256]>(0x018060E8);
    static auto& g_Fps = AddrAsRef<float>(0x005A4018);
    static auto& g_Framerate = AddrAsRef<float>(0x005A4014);
    static auto& g_MinFramerate = AddrAsRef<float>(0x005A4024);
    static auto& g_LevelName = AddrAsRef<String>(0x00645FDC);
    static auto& g_LevelFilename = AddrAsRef<String>(0x00645FE4);
    static auto& g_LevelAuthor = AddrAsRef<String>(0x00645FEC);
    static auto& g_LevelDate = AddrAsRef<String>(0x00645FF4);
    static auto& g_BigFontId = AddrAsRef<int>(0x006C74C0);
    static auto& g_LargeFontId = AddrAsRef<int>(0x0063C05C);
    static auto& g_MediumFontId = AddrAsRef<int>(0x0063C060);
    static auto& g_SmallFontId = AddrAsRef<int>(0x0063C068);
    static auto& g_DirectInputDisabled = AddrAsRef<bool>(0x005A4F88);
    static auto& g_DefaultPlayerWeapon = AddrAsRef<String>(0x007C7600);
    static auto& g_active_cutscene = AddrAsRef<void*>(0x00645320);

    static auto& RfBeep = AddrAsRef<void(unsigned u1, unsigned u2, unsigned u3, float f_volume)>(0x00505560);
    static auto& GetFileExt = AddrAsRef<char *(const char *path)>(0x005143F0);
    static auto& SplitScreenStart = AddrAsRef<void()>(0x00480D30);
    static auto& SetNextLevelFilename = AddrAsRef<void(String::Pod str_filename, String::Pod str_second)>(0x0045E2E0);
    static auto& DemoLoadLevel = AddrAsRef<void(const char *level_file_name)>(0x004CC270);
    static auto& SetCursorVisible = AddrAsRef<void(bool visible)>(0x0051E680);
    static auto& CutsceneIsActive = AddrAsRef<bool()>(0x0045BE80);
    static auto& Timer__GetTimeLeftMs = AddrAsRef<int __thiscall(void* timer)>(0x004FA420);

    /* Strings Table */
    namespace strings {
        static const auto &array = AddrAsRef<char*[1000]>(0x007CBBF0);
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

    static auto& Free = AddrAsRef<void(void *mem)>(0x00573C71);
    static auto& Malloc = AddrAsRef<void*(uint32_t cb_size)>(0x00573B37);
}

#pragma pack(pop)
