#pragma once

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstdarg>
#include <windows.h>
#include <d3d8.h>
#include <patch_common/MemUtils.h>
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
            auto fun_ptr = reinterpret_cast<String& (__thiscall*)(String& self)>(0x004FF3B0);
            fun_ptr(*this);
        }

        String(const char* c_str)
        {
            auto fun_ptr = reinterpret_cast<String&(__thiscall*)(String & self, const char* c_str)>(0x004FF3D0);
            fun_ptr(*this, c_str);
        }

        String(const String& str)
        {
            // Use Pod cast operator to make a clone
            m_pod = str;
        }

        ~String()
        {
            auto fun_ptr = reinterpret_cast<void(__thiscall*)(String & self)>(0x004FF470);
            fun_ptr(*this);
        }

        operator const char*() const
        {
            return CStr();
        }

        operator Pod() const
        {
            // Make a copy
            auto fun_ptr = reinterpret_cast<Pod&(__thiscall*)(Pod & self, const Pod& str)>(0x004FF410);
            Pod pod_copy;
            fun_ptr(pod_copy, m_pod);
            return pod_copy;
        }

        String& operator=(const String& other)
        {
            auto fun_ptr = reinterpret_cast<String&(__thiscall*)(String & self, const String& str)>(0x004FFA20);
            fun_ptr(*this, other);
            return *this;
        }

        const char *CStr() const
        {
            auto fun_ptr = reinterpret_cast<const char*(__thiscall*)(const String& self)>(0x004FF480);
            return fun_ptr(*this);
        }

        int Size() const
        {
            auto fun_ptr = reinterpret_cast<int(__thiscall*)(const String& self)>(0x004FF490);
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
        int end_time_ms;
    };

    struct Color
    {
        int val;
    };

    template<typename T = char>
    class DynamicArray
    {
    private:
        int size;
        int capacity;
        T *data;

    public:
        int Size()
        {
            return size;
        }

        T& Get(int index)
        {
            return data[index];
        }
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
        const char *cmd_name;
        const char *descr;
        void(*func)();

        inline static const auto Init =
            reinterpret_cast<void(__thiscall*)(rf::DcCommand* self, const char* cmd, const char* descr, DcCmdHandler handler)>(
                0x00509A70);
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

    static auto& DcPrint = AddrAsRef<void(const char *text, const uint32_t *color)>(0x00509EC0);
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

    //static auto& DcCommands = AddrAsRef<DcCommand*[30]>(0x01775530);
    static auto& dc_num_commands = AddrAsRef<uint32_t>(0x0177567C);

    static auto& dc_run = AddrAsRef<uint32_t>(0x01775110);
    static auto& dc_help = AddrAsRef<uint32_t>(0x01775224);
    static auto& dc_status = AddrAsRef<uint32_t>(0x01774D00);
    static auto& dc_arg_type = AddrAsRef<uint32_t>(0x01774D04);
    static auto& dc_str_arg = AddrAsRef<char[256]>(0x0175462C);
    static auto& dc_int_arg = AddrAsRef<int>(0x01775220);
    static auto& dc_float_arg = AddrAsRef<float>(0x01754628);
    static auto& dc_cmd_line = AddrAsRef<char[256]>(0x01775330);
    static auto& dc_cmd_line_len = AddrAsRef<uint32_t>(0x0177568C);

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

    /* Graphics */

    struct GrScreen
    {
        int signature;
        int max_width;
        int max_height;
        int mode;
        int window_mode;
        int field_14;
        float aspect;
        int field_1c;
        int bits_per_pixel;
        int bytes_ber_pixel;
        int field_28;
        int offset_x;
        int offset_y;
        int viewport_width;
        int viewport_height;
        int max_tex_width_unk3_c;
        int max_tex_height_unk40;
        int clip_left;
        int clip_right;
        int clip_top;
        int clip_bottom;
        int current_color;
        int current_bitmap;
        int field_5c;
        int fog_mode;
        int fog_color;
        float fog_near;
        float fog_far;
        float field_70;
        int field_74;
        float field_78;
        float field_7c;
        float field_80;
        int field_84;
        int field_88;
        int field_8c;
    };

    enum GrScreenMode
    {
        GR_NONE = 0,
        GR_DIRECT3D = 102,
    };

    struct GrVertex
    {
        Vector3 pos3_d;
        Vector3 screen_pos;
        int clip_flags;
        float u0;
        float v0;
        float u1;
        float v1;
        int color;
    };

    struct GrLockData
    {
        int bm_handle;
        int section_idx;
        BmPixelFormat pixel_format;
        uint8_t *bits;
        int width;
        int height;
        int pitch;
        int field_1c;
    };
    static_assert(sizeof(GrLockData) == 0x20);

    enum GrTextAlignment
    {
        GR_ALIGN_LEFT = 0,
        GR_ALIGN_CENTER = 1,
        GR_ALIGN_RIGHT = 2,
    };

    static auto& gr_d3d = AddrAsRef<IDirect3D8*>(0x01CFCBE0);
    static auto& gr_d3d_device = AddrAsRef<IDirect3DDevice8*>(0x01CFCBE4);
    static auto& gr_d3d_pp = AddrAsRef<D3DPRESENT_PARAMETERS>(0x01CFCA18);
    static auto& gr_screen = AddrAsRef<GrScreen>(0x017C7BC0);

    static auto& gr_line_material = AddrAsRef<uint32_t>(0x01775B00);
    static auto& gr_rect_material = AddrAsRef<uint32_t>(0x17756C0);
    static auto& gr_text_material = AddrAsRef<uint32_t>(0x17C7C5C);
    static auto& gr_bitmap_material = AddrAsRef<uint32_t>(0x017756BC);
    static auto& gr_image_material = AddrAsRef<uint32_t>(0x017756DC);

    static auto& GrGetMaxWidth = AddrAsRef<int()>(0x0050C640);
    static auto& GrGetMaxHeight = AddrAsRef<int()>(0x0050C650);
    static auto& GrGetViewportWidth = AddrAsRef<unsigned()>(0x0050CDB0);
    static auto& GrGetViewportHeight = AddrAsRef<unsigned()>(0x0050CDC0);
    static auto& GrSetColor = AddrAsRef<void(unsigned r, unsigned g, unsigned b, unsigned a)>(0x0050CF80);
    static auto& GrReadBackBuffer = AddrAsRef<int(int x, int y, int width, int height, void *buffer)>(0x0050DFF0);
    static auto& GrFlushBuffers = AddrAsRef<void()>(0x00559D90);

    static auto& GrDrawRect = AddrAsRef<void(unsigned x, unsigned y, unsigned cx, unsigned cy, unsigned material)>(0x0050DBE0);
    static auto& GrDrawImage = AddrAsRef<void(int bm_handle, int x, int y, int material)>(0x0050D2A0);
    static auto& GrDrawBitmapStretched = AddrAsRef<void(int bm_handle, int dst_x, int dst_y, int dst_w, int dst_h, int src_x, int src_y, int src_w, int src_h, float a10, float a11, int material)>(0x0050D250);
    static auto& GrDrawText = AddrAsRef<void(unsigned x, unsigned y, const char *text, int font, unsigned material)>(0x0051FEB0);
    static auto& GrDrawAlignedText = AddrAsRef<void(GrTextAlignment align, unsigned x, unsigned y, const char *text, int font, unsigned material)>(0x0051FE50);

    static auto& GrFitText = AddrAsRef<String* (String* result, String::Pod str, int cx_max)>(0x00471EC0);
    static auto& GrLoadFont = AddrAsRef<int(const char *file_name, int a2)>(0x0051F6E0);
    static auto& GrGetFontHeight = AddrAsRef<unsigned(int font_id)>(0x0051F4D0);
    static auto& GrGetTextWidth = AddrAsRef<void(int *out_width, int *out_height, const char *text, int text_len, int font_id)>(0x0051F530);

    static auto& GrLock = AddrAsRef<char(int bm_handle, int section_idx, GrLockData *data, int a4)>(0x0050E2E0);
    static auto& GrUnlock = AddrAsRef<void(GrLockData *data)>(0x0050E310);

    /* User Interface (UI) */

    struct UiGadget
    {
        void **vtbl;
        UiGadget *parent;
        bool highlighted;
        bool enabled;
        int x;
        int y;
        int w;
        int h;
        int key;
        void(*on_click)();
        void(*on_mouse_btn_down)();
    };

    static auto& UiMsgBox = AddrAsRef<void(const char *title, const char *text, void(*callback)(), bool input)>(0x004560B0);
    using UiDialogCallbackPtr = void (*)();
    static auto& UiCreateDialog =
        AddrAsRef<void(const char *title, const char *text, unsigned num_buttons, const char *ppsz_btn_titles[],
                       UiDialogCallbackPtr callbacks[], unsigned unknown1, unsigned unknown2)>(0x004562A0);
    static auto& UiGetGadgetFromPos = AddrAsRef<int(int x, int y, UiGadget * const gadgets[], int num_gadgets)>(0x00442ED0);

    /* Parse */

    struct StrParser
    {
        bool OptionalString(const char *str)
        {
            return reinterpret_cast<bool(__thiscall*)(StrParser*, const char*)>(0x005125C0)(this, str);
        }

        bool RequiredString(const char *str)
        {
            return reinterpret_cast<bool(__thiscall*)(StrParser*, const char*)>(0x005126A0)(this, str);
        }

        bool GetBool()
        {
            return reinterpret_cast<bool(__thiscall*)(StrParser*)>(0x005126F0)(this);
        }

        int GetInt()
        {
            return reinterpret_cast<int(__thiscall*)(StrParser*)>(0x00512750)(this);
        }

        unsigned GetUInt()
        {
            return reinterpret_cast<int(__thiscall*)(StrParser*)>(0x00512840)(this);
        }

        float GetFloat()
        {
            return reinterpret_cast<int(__thiscall*)(StrParser*)>(0x00512920)(this);
        }
    };

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

    static auto& ChatPrint = AddrAsRef<void(String::Pod text, ChatMsgColor color, String::Pod prefix)>(0x004785A0);
    static auto& ChatSay = AddrAsRef<void(const char *msg, bool is_team_msg)>(0x00444150);

    /* File System */

    static auto& PackfileLoad = AddrAsRef<int(const char *file_name, const char *dir)>(0x0052C070);
    static auto& FsAddDirectoryEx = AddrAsRef<int(const char *dir, const char *ext_list, bool unknown)>(0x00514070);

    /* Network */

    struct NwAddr
    {
        uint32_t ip_addr;
        uint16_t port;
        uint16_t unused;

        bool operator==(const NwAddr &other) const
        {
            return ip_addr == other.ip_addr && port == other.port;
        }

        bool operator!=(const NwAddr &other) const
        {
            return !(*this == other);
        }
    };

    struct NwStats
    {
        int num_total_sent_bytes;
        int num_total_recv_bytes;
        int num_sent_bytes_unk_idx_f8[30];
        int num_recv_bytes_unk_idx_f8[30];
        int unk_idx_f8;
        int num_sent_bytes_unk_idx1_ec[30];
        int num_recv_bytes_unk_idx1_ec[30];
        int unk_idx1_ec;
        Timer field_1f0;
        int num_obj_update_packets_sent;
        int num_obj_update_packets_recv;
        int field_1fc[3];
        int num_sent_bytes_per_packet[55];
        int num_recv_bytes_per_packet[55];
        int num_packets_sent[55];
        int num_packets_recv[55];
        Timer field_578;
        int field_57c;
        int field_580;
        int field_584;
        Timer field_588;
        int field_58c;
    };

    struct PlayerNetData
    {
        NwAddr addr;
        NwPlayerFlags flags;
        int field_c;
        int connection_id;
        uint8_t player_id;
        uint8_t unused[3];
        int field_18;
        NwStats stats;
        int ping;
        float field_5b0;
        char packet_buf[512];
        int cb_packet_buf;
        char out_reliable_buf[512];
        int cb_out_reliable_buf;
        int connection_speed;
        int field_9c0;
        Timer field_9c4;
    };
    static_assert(sizeof(PlayerNetData) == 0x9C8, "invalid size");

    static auto& NwSendNotReliablePacket =
        AddrAsRef<void(const NwAddr &addr, const void *packet, unsigned cb_packet)>(0x0052A080);
    static auto& NwSendReliablePacket =
        AddrAsRef<void(Player *player, const uint8_t *data, unsigned int num_bytes, int a4)>(0x00479480);
    static auto& NwSendReliablePacketToAll =
        AddrAsRef<void( const uint8_t *data, unsigned int num_bytes, int a4)>(0x004795A0);
    static auto& NwAddrToStr = AddrAsRef<void(char *dest, int cb_dest, const NwAddr& addr)>(0x00529FE0);
    static auto& NwGetPlayerFromAddr = AddrAsRef<Player*(const NwAddr& addr)>(0x00484850);
    static auto& NwCompareAddr = AddrAsRef<int(const NwAddr &addr1, const NwAddr &addr2, bool check_port)>(0x0052A930);
    static auto& GetPlayerById = AddrAsRef<Player*(uint8_t id)>(0x00484890);

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
        EntityObj *camera_entity;
        Player *player;
        CameraType type;
    };

    static auto& CameraSetFirstPerson = AddrAsRef<void(Camera *camera)>(0x0040DDF0);
    static auto& CameraSetFreelook = AddrAsRef<void(Camera *camera)>(0x0040DCF0);

    /* Config */

    struct ControlConfigItem
    {
        int16_t default_scan_codes[2];
        int16_t default_mouse_btn_id;
        int16_t field_6;
        int field_8;
        String name;
        int16_t scan_codes[2];
        int16_t mouse_btn_id;
        int16_t field_1a;
    };

    struct ControlConfig
    {
        float mouse_sensitivity;
        int mouse_look;
        int field_ec;
        ControlConfigItem keys[128];
        int ctrl_count;
        int field_ef4;
        int field_ef8;
        int field_efc;
        int field_f00;
        char field_f04;
        char mouse_y_invert;
        char field_e22;
        char field_e23;
        int field_f08;
        int field_f0c;
        int field_f10;
        int field_f14;
        int field_f18;
        int field_f1c;
        int field_f20;
        int field_f24;
        int field_f28;
    };

    struct PlayerConfig
    {
        ControlConfig controls;
        int field_f2c;
        int field_f30;
        char field_f34;
        char field_e51;
        char first_person_weapon_visible;
        char field_e53;
        char crouch_stay;
        char field_f39;
        char field_f3a;
        char hud_visible;
        char field_f3c;
        char field_e59;
        char field_e5a;
        char field_e5b;
        char autoswitch_weapons;
        char autoswitch_explosives;
        char shadows_enabled;
        char decals_enabled;
        char dynamic_lightining_enabled;
        char field_f45;
        char field_f46;
        char field_f47;
        char filtering_level;
        char field_f49;
        char field_f4a;
        char field_f4b;
        int detail_level;
        int textures_resolution_level;
        int character_detail_level;
        float field_f58;
        int mp_character;
        char name[12];
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
        int next_weapon_cls_id;
        Timer weapon_switch_timer;
        Timer unk_reload_timer;
        int field_f8c;
        Timer unk_timer_f90;
        char in_scope_view;
        char field_f95;
        char field_f96;
        char field_f97;
        float scope_zoom;
        char field_f9c;
        char field_1d;
        char field_1e;
        char field_1f;
        int field_fa0;
        Timer field_fa4;
        Timer timer_f_a8;
        Timer field_fac;
        char railgun_scanner;
        char scanner_view;
        Color clr_unk_r;
        char field_fb6;
        char field_fb7;
        int field_fb8;
        int field_fbc;
        float field_fc0;
        Vector3 field_fc4;
        Matrix3 field_fd0;
        Matrix3 rot_fps_weapon;
        Vector3 pos_fps_weapon;
        int field_1024;
        float field_1028;
        int field_102c;
        float field_1030;
        int pivot1_prop_id;
        Timer field_1038;
        int field_103c;
        int field_1040;
        int remote_charge_visible;
        Vector3 field_1048;
        Matrix3 field_1054;
        int field_1078;
        Timer field_107c;
    };
    static_assert(sizeof(PlayerWeaponInfo) == 0x100, "invalid size");

    struct Player1094
    {
        Vector3 field_1094;
        Matrix3 field_10a0;
    };

    struct Player
    {
        Player *next;
        Player *prev;
        String name;
        PlayerFlags flags;
        int entity_handle;
        int entity_cls_id;
        Vector3 field_1c;
        int field_28;
        PlayerStats *stats;
        char blue_team;
        char collide;
        char field_32;
        char field_33;
        AnimMesh *fpgun_mesh;
        AnimMesh *last_fpgun_mesh;
        Timer timer3_c;
        int fpgun_muzzle_props[2];
        int fpgun_ammo_digit1_prop;
        int fpgun_ammo_digit2_prop;
        int field_50[24];
        char field_b0;
        char is_crouched;
        char field_b2;
        char field_b3;
        int view_obj_handle;
        Timer weapon_switch_timer2;
        Timer use_key_timer;
        Timer field_c0;
        Camera *camera;
        int x_viewport;
        int y_viewport;
        int cx_viewport;
        int cy_viewport;
        float fov;
        int view_mode;
        int field_e0;
        PlayerConfig config;
        int field_f6c;
        int field_f70;
        int field_f74;
        int field_f78;
        int field_f7c;
        PlayerWeaponInfo weapon_info;
        int fpgun_weapon_id;
        int field_1084;
        int scanner_bm_handle;
        int field_108c[2];
        Player1094 field_1094;
        int field_10c4[3];
        Color hit_scr_color;
        int hit_scr_alpha;
        int field_10d8;
        int field_10dc;
        int field_10e0;
        int field_10e4;
        int field_10e8[26];
        int sound1150_not_sure;
        int pref_weapons[32];
        float field_11d4;
        float field_11d8;
        void(__cdecl *field_11dc)(Player *);
        float field_11e0[4];
        int field_11f0;
        float field_11f4;
        int field_11f8;
        int field_11fc;
        PlayerNetData *nw_data;
    };
    static_assert(sizeof(Player) == 0x1204, "invalid size");

    static auto& player_list = AddrAsRef<Player*>(0x007C75CC);
    static auto& local_player = AddrAsRef<Player*>(0x007C75D4);

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
        float field_8c;
        int field_90;
        int field_94;
        float mass;
        Matrix3 field_9c;
        Matrix3 field_c0;
        Vector3 pos;
        Vector3 new_pos;
        Matrix3 yaw_rot;
    };

    struct WaterSplashUnk
    {
        Vector3 field_1b4;
        Vector3 field_1c0;
        float field_1cc;
        int field_1d0;
        float field_1d4;
        Vector3 field_1d8;
        int unk_entity_handle;
        int field_1e8;
        int water_splash_1_ec;
        int field_3c;
        int field_40;
    };

    struct PhysicsInfo
    {
        float elasticity;
        float field_8c;
        float friction;
        int field_94;
        float mass;
        Matrix3 field_9c;
        Matrix3 field_c0;
        Vector3 pos;
        Vector3 new_pos;
        Matrix3 yaw_rot;
        Matrix3 field_120;
        Vector3 vel;
        Vector3 rot_change_unk;
        Vector3 field_15c;
        Vector3 field_168;
        Vector3 rot_change_unk_delta;
        float radius;
        DynamicArray<> field_184;
        Vector3 aabb_min;
        Vector3 aabb_max;
        PhysicsFlags flags;
        int flags_splash_1_ac;
        float frame_time;
        WaterSplashUnk water_splash_unk;
    };
    static_assert(sizeof(PhysicsInfo) == 0x170, "invalid size");

    struct Object
    {
        void *room;
        Vector3 last_pos_in_room;
        Object *next_obj;
        Object *prev_obj;
        String name;
        int uid;
        ObjectType type;
        int team;
        int handle;
        int owner_entity_unk_handle;
        float life;
        float armor;
        Vector3 pos;
        Matrix3 orient;
        Vector3 last_pos;
        float radius;
        ObjectFlags flags;
        AnimMesh *anim_mesh;
        int field_84;
        PhysicsInfo phys_info;
        int friendliness;
        int material;
        int parent_handle;
        int unk_prop_id_204;
        Vector3 field_208;
        Matrix3 mat214;
        Vector3 pos3;
        Matrix3 rot2;
        int *emitter_list_head;
        int field_26c;
        char killer_id;
        char pad[3]; // FIXME
        int multi_handle;
        int anim;
        int field_27c;
        Vector3 field_280;
    };
    static_assert(sizeof(Object) == 0x28C, "invalid size");

    struct EventObj
    {
        int vtbl;
        Object _super;
        int event_type;
        float delay;
        int field_298;
        int link_list;
        int field_2a0;
        int field_2a4;
        int field_2a8;
        int field_2ac;
        int field_2b0;
    };

    struct ItemObj
    {
        Object _super;
        ItemObj *next;
        ItemObj *prev;
        int field_294;
        int item_cls_id;
        String field_29c;
        int field_2a4;
        int field_2a8;
        int field_2ac;
        char field_2b0[12];
        int field_2bc;
        int field_2c0;
    };

    struct TriggerObj
    {
        Object _super;
        TriggerObj *next;
        TriggerObj *prev;
        int shape;
        Timer resets_after_timer;
        int resets_after_ms;
        int resets_counter;
        int resets_times;
        float last_activation_time;
        int key_item_cls_id;
        int flags;
        String field_2b4;
        int field_2bc;
        int airlock_room_uid;
        int activated_by;
        Vector3 box_size;
        DynamicArray<> links;
        Timer activation_failed_timer;
        int activation_failed_entity_handle;
        Timer field_2e8;
        char one_way;
        char padding[3];
        float button_active_time_seconds;
        Timer field_2f4;
        float inside_time_seconds;
        Timer inside_timer;
        int attached_to_uid;
        int use_clutter_uid;
        char team;
        char padding2[3];
    };
    static_assert(sizeof(TriggerObj) == 0x30C, "invalid size");

    static auto& ObjGetFromUid = AddrAsRef<Object *(int uid)>(0x0048A4A0);

    /* Entity */

    typedef void EntityClass;

    struct MoveModeTbl
    {
        char available;
        char unk, unk2, unk3;
        int id;
        int move_ref_x;
        int move_ref_y;
        int move_ref_z;
        int rot_ref_x;
        int rot_ref_y;
        int rot_ref_z;
    };

    struct SWeaponSelection
    {
        EntityClass *entity_cls;
        int weapon_cls_id;
        int weapon_cls_id2;
    };

    struct SEntityMotion
    {
        Vector3 rot_change;
        Vector3 pos_change;
        int field_18;
        float field_1c;
        float field_20;
    };

    struct EntityWeapon2E8InnerUnk
    {
        Vector3 field_0;
        Vector3 field_c;
        int field_18;
        int field_1c;
        float field_20;
        int field_24;
        DynamicArray<> field_28;
        char field_34;
        char field_35;
        int16_t field_36;
        int field_38;
        int field_3c;
        int field_40;
        int field_44;
        Matrix3 field_48;
        int field_6c;
        DynamicArray<> field_70;
    };

    struct EntityWeapon2E8
    {
        int field_0;
        int field_4;
        int field_8[5];
        EntityWeapon2E8InnerUnk field_1c;
        EntityWeapon2E8InnerUnk field_98;
        int field_114;
        int field_118;
        Timer timer_11_c;
        int field_120;
        int field_124;
        int field_128;
        int field_12c;
        int entity_unk_handle;
        Timer timer_134;
        Vector3 field_138;
        int field_144;
        int field_148;
        Vector3 field_14c;
        int field_158;
        int field_15c;
        int field_160;
        Vector3 field_164;
        int field_170;
        Vector3 field_174;
    };

    struct EntityWeaponInfo
    {
        EntityObj *entity;
        int weapon_cls_id;
        int weapon_cls_id2;
        int clip_ammo[32];
        int weapons_ammo[64];
        char field_18c[64];
        char field_1cc[64];
        DynamicArray<> field_20c;
        Timer fire_wait_timer;
        Timer alt_fire_wait_in_veh_timer;
        Timer impact_delay_timer[2];
        int field_228;
        Timer timer_22c;
        Timer timer_230;
        Timer timer_234;
        Timer timer_238;
        Timer timer_23c;
        Vector3 field_240;
        int field_24c;
        Timer timer_250;
        Timer timer_254;
        Timer timer_258;
        Timer timer_25c;
        int field_260;
        Timer timer_264;
        int field_268;
        int field_26c;
        int field_270;
        Timer unk_weapon_timer;
        Timer field_278;
        int cooperation;
        int ai_mode;
        int field_284;
        int unk_time288;
        int field_28c;
        int field_290;
        Timer timer_294;
        float create_time;
        char field_29c;
        char field_29d;
        int16_t field_29e;
        Timer timer_2a0;
        Timer timer_2a4;
        Timer timer_2a8;
        int field_2AC;
        int field_2B0;
        int submode;
        int field_2B8;
        int submode_change_time;
        int unk_obj_handle;
        Vector3 field_2c4;
        Timer field_2d0;
        int field_2d4;
        int field_2d8;
        Timer field_2dc;
        Timer field_2e0;
        int field_2e4;
        EntityWeapon2E8 field_2e8;
        SEntityMotion motion_change;
        Timer field_48c;
        Vector3 field_490;
        float last_dmg_time;
        int field_4a0;
        Timer field_4a4;
        int field_4a8[3];
        Timer field_4b4;
        Timer field_4b8;
        int unk_handle;
        int field_4c0;
        Timer field_4c4;
        int field_4c8;
        int field_4cc;
        Timer field_4d0;
        Timer field_4d4;
        Timer field_4d8;
        Timer field_4dc;
        Timer field_4e0;
        Timer field_4e4;
        Timer field_4e8;
        Vector3 field_4ec;
        Timer timer_4f8;
        Timer timer_4fc;
        Vector3 pos_delta;
        int field_50c;
        float last_rot_time;
        float last_move_time;
        float last_fire_level_time;
        int field_51c_;
        float movement_radius;
        float field_524_;
        int field_528;
        int field_52c;
        int flags;
    };
    static_assert(sizeof(EntityWeaponInfo) == 0x534, "invalid size");

    struct EntityCameraInfo
    {
        int field_860;
        Vector3 rot_yaw;
        Vector3 rot_yaw_delta;
        Vector3 rot_pitch;
        Vector3 rot_pitch_delta;
        Vector3 field_894;
        Vector3 field_8a0;
        int field_8ac;
        int field_8b0;
        float camera_shake_dist;
        float camera_shake_time;
        Timer camera_shake_timer;
    };

    struct EntityObj
    {
        Object _super;
        EntityObj *next;
        EntityObj *prev;
        EntityClass *cls;
        int class_id;
        EntityClass *cls2;
        EntityWeaponInfo weapon_info;
        Vector3 view_pos;
        Matrix3 view_orient;
        int unk_ambient_sound;
        int field_808;
        int move_snd;
        EntityFlags entity_flags;
        EntityPowerups powerups;
        Timer unk_timer818;
        int launch_snd;
        int field_820;
        int16_t kill_type;
        int16_t field_826;
        int field_828;
        int field_82c;
        Timer field_830;
        int force_state_anim;
        int field_834[4];
        AnimMesh *field_848;
        AnimMesh *field_84c;
        int nano_shield;
        int snd854;
        MoveModeTbl *movement_mode;
        Matrix3 *field_85c;
        EntityCameraInfo camera_info;
        float max_vel;
        int max_vel_modifier_type;
        int field_8c8;
        DynamicArray<> interface_props;
        DynamicArray<> unk_anim_mesh_array8_d8;
        int field_8e4[92];
        int unk_anims[180];
        int field_d24[129];
        int field_f28[200];
        int field_1248[6];
        int field_1260;
        int field_1264;
        int field_1268[65];
        Timer timer_136_c;
        Timer timer1370;
        Timer shell_eject_timer;
        int field_1378;
        Timer field_137c;
        int field_1380;
        int field_1384;
        int field_1388;
        int current_state;
        int next_state;
        float field_1394;
        float field_1398;
        Timer field_139c;
        Timer field_13a0;
        Timer weapon_drain_timer;
        int reload_ammo;
        int reload_clip_ammo;
        int reload_weapon_cls_id;
        float field_13b4;
        Vector3 field_13b8;
        int field_13c4[2];
        Timer field_13cc;
        int field_13d0[2];
        int field_13d8;
        int field_13dc;
        Vector3 field_13e0;
        int field_13ec;
        int field_13f0;
        int unk_bone_id13_f4;
        void *field_13f8;
        Timer timer13_fc;
        int speaker_sound;
        int field_1404;
        float unk_count_down_time1408;
        Timer unk_timer140_c;
        AnimMesh *field_1410;
        Timer splash_in_counter;
        DynamicArray<> field_1418;
        int field_1424;
        int field_1428;
        int field_142c;
        Player *local_player;
        Vector3 pitch_min;
        Vector3 pitch_max;
        int killer_entity_handle;
        int riot_shield_clutter_handle;
        int field_1454;
        Timer field_1458;
        int unk_clutter_handles[2];
        float time;
        int field_1468;
        int unk_entity_handle;
        int field_1470;
        Color ambient_color;
        int field_1478;
        int field_147c;
        int field_1480;
        int field_1484;
        int field_1488;
        AnimMesh *respawn_vfx;
        Timer field_1490;
    };
    static_assert(sizeof(EntityObj) == 0x1494, "invalid size");

    static auto& EntityGetFromHandle = AddrAsRef<EntityObj*(uint32_t handle)>(0x00426FC0);

    /* Weapons */

    struct WeaponStateAction
    {
        String name;
        String anim_name;
        int anim_id;
        int sound_id;
        int sound_handle;
    };

    struct WeaponClass
    {
        String name;
        String display_name;
        String v3d_filename;
        int v3d_type;
        String embedded_v3d_filename;
        int ammo_type;
        String third_person_v3d;
        AnimMesh *third_person_mesh_not_sure;
        int muzzle1_prop_id;
        int third_person_grip1_mesh_prop;
        int third_person_muzzle_flash_glare;
        String first_person_mesh;
        void *mesh;
        Vector3 first_person_offset;
        Vector3 first_person_offset_ss;
        int first_person_muzzle_flash_bitmap;
        float first_person_muzzle_flash_radius;
        int first_person_alt_muzzle_flash_bitmap;
        float first_person_alt_muzzle_flash_radius;
        float first_person_fov;
        float splitscreen_fov;
        int num_projectiles;
        int clip_size_sp;
        int clip_size_mp;
        int clip_size;
        float clip_reload_time;
        float clip_drain_time;
        int bitmap;
        int trail_emitter;
        float head_radius;
        float tail_radius;
        float head_len;
        int is_secondary;
        float collision_radius;
        int lifetime;
        float lifetime_sp;
        float lifetime_multi;
        float mass;
        float velocity;
        float velocity_multi;
        float velocity_sp;
        float lifetime_mul_vel;
        float fire_wait;
        float alt_fire_wait;
        float spread_degrees_sp;
        float spread_degrees_multi;
        int spread_degrees;
        float alt_spread_degrees_sp;
        float alt_spread_degrees_multi;
        int alt_spread_degrees;
        float ai_spread_degrees_sp;
        float ai_spread_degrees_multi;
        int ai_spread_degrees;
        float ai_alt_spread_degrees_sp;
        float ai_alt_spread_degrees_multi;
        int ai_alt_spread_degrees;
        float damage;
        float damage_multi;
        float alt_damage;
        float alt_damage_multi;
        float ai_dmg_scale_sp;
        float ai_dmg_scale_mp;
        int ai_dmg_scale;
        int field_124;
        int field_128;
        int field_12c;
        float turn_time;
        float view_cone;
        float scanning_range;
        float wakeup_time;
        float drill_time;
        float drill_range;
        float drill_charge;
        float crater_radius;
        float impact_delay[2];
        float alt_impact_delay[2];
        int launch;
        int alt_launch;
        int silent_launch;
        int underwater_launch;
        int launch_fail;
        int fly_sound;
        int material_impact_sound[30];
        int near_miss_snd;
        int near_miss_underwater_snd;
        int geomod_sound;
        int start_sound;
        float start_delay;
        int stop_sound;
        float damage_radius;
        float damage_radius_sp;
        float damage_radius_multi;
        float glow;
        float field_218;
        int glow2;
        int field_220;
        int field_224;
        int field_228;
        int muzzle_flash_light;
        int field_230;
        int muzzle_flash_light2;
        int field_238;
        int field_23c;
        int field_240;
        int hud_icon;
        int hud_reticle_texture;
        int hud_zoomed_reticle_texture;
        int hud_locked_reticle_texture;
        int zoom_sound;
        int max_ammo_sp;
        int max_ammo_mp;
        int max_ammo;
        int flags;
        int flags2;
        int field_26c;
        int field_270;
        int tracer_frequency;
        int field_278;
        float piercing_power;
        float ricochet_angle;
        int ricochet_bitmap;
        Vector3 ricochet_size;
        float thrust_lifetime;
        int num_states;
        int num_actions;
        WeaponStateAction states[3];
        WeaponStateAction actions[7];
        int field_3b8[35];
        int burst_count;
        float burst_delay;
        int burst_launch_sound;
        int impact_vclips_count;
        int impact_vclips[3];
        String impact_vclip_names[3];
        float impact_vclips_radius_list[3];
        int decal_texture;
        Vector3 decal_size;
        int glass_decal_texture;
        Vector3 glass_decal_size;
        int fpgun_shell_eject_prop_id;
        Vector3 shells_ejected_base_dir;
        float shell_eject_velocity;
        String shells_ejected_v3d;
        int shells_ejected_custom_sound_set;
        float primary_pause_time_before_eject;
        int fpgun_clip_eject_prop_id;
        String clips_ejected_v3d;
        float clips_ejected_drop_pause_time;
        int clips_ejected_custom_sound_set;
        float reload_zero_drain;
        float camera_shake_dist;
        float camera_shake_time;
        String silencer_v3d;
        int field_4f0_always0;
        int fpgun_silencer_prop_id;
        String spark_vfx;
        int field_500always0;
        int fpgun_thruster_prop_id;
        int field_508;
        float ai_attack_range1;
        float ai_attack_range2;
        float field_514;
        int fpgun_weapon_prop_id;
        int field_51c_always1neg;
        int weapon_type;
        String weapon_icon;
        int damage_type;
        int cycle_pos;
        int pref_pos;
        float fine_aim_reg_size;
        float fine_aim_reg_size_ss;
        String tracer_effect;
        int field_548always0;
        float multi_b_box_size_factor;
    };
    static_assert(sizeof(WeaponClass) == 0x550, "invalid size");

    static auto& weapon_classes = AddrAsRef<WeaponClass[64]>(0x0085CD08);
    static auto& riot_stick_cls_id = AddrAsRef<int32_t>(0x00872468);
    static auto& remote_charge_cls_id = AddrAsRef<int32_t>(0x0087210C);

    /* Window */

    typedef void(*MsgHandlerPtr)(UINT, WPARAM, LPARAM);
    static auto &AddMsgHandler = AddrAsRef<unsigned(MsgHandlerPtr)>(0x00524AE0);

    static auto& msg_handlers = AddrAsRef<MsgHandlerPtr[32]>(0x01B0D5A0);
    static auto& num_msg_handlers = AddrAsRef<uint32_t>(0x01B0D760);

    static auto& main_wnd = AddrAsRef<HWND>(0x01B0D748);
    static auto& is_main_wnd_active = AddrAsRef<uint8_t>(0x01B0D750);
    static auto& close_app_req = AddrAsRef<uint8_t>(0x01B0D758);
    static auto& mouse_initialized = AddrAsRef<uint8_t>(0x01885461);
    static auto& num_redraw_server = AddrAsRef<uint32_t>(0x01775698);

    /* Network Game */

    enum MpGameMode {
        GM_DM = 0,
        GM_CTF = 1,
        GM_TEAMDM = 2,
    };

    static auto& MpGetGameMode = AddrAsRef<MpGameMode()>(0x00470770);
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
    static auto& MultiChangeLevel = AddrAsRef<void(const char* filename)>(0x0047BF50);

    static auto& serv_addr = AddrAsRef<NwAddr>(0x0064EC5C);
    static auto& serv_name = AddrAsRef<String>(0x0064EC28);
    static auto& is_net_game = AddrAsRef<uint8_t>(0x0064ECB9);
    static auto& is_local_net_game = AddrAsRef<uint8_t>(0x0064ECBA);
    static auto& is_dedicated_server = AddrAsRef<uint32_t>(0x01B0D75C);
    static auto& game_options = AddrAsRef<uint32_t>(0x0064EC40);
    static auto& level_rotation_idx = AddrAsRef<int>(0x0064EC64);
    static auto& server_level_list = AddrAsRef<DynamicArray<String>>(0x0064EC68);

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
        uint8_t *unk;
        int w;
        int h;
        uint8_t *buf;
        int bm_handle;
        int unk2;
    };

    static auto& root_path = AddrAsRef<char[256]>(0x018060E8);
    static auto& current_fps = AddrAsRef<float>(0x005A4018);
    static auto& frametime = AddrAsRef<float>(0x005A4014);
    static auto& min_framerate = AddrAsRef<float>(0x005A4024);
    static auto& level_name = AddrAsRef<String>(0x00645FDC);
    static auto& level_filename = AddrAsRef<String>(0x00645FE4);
    static auto& level_author = AddrAsRef<String>(0x00645FEC);
    static auto& big_font_id = AddrAsRef<int>(0x006C74C0);
    static auto& large_font_id = AddrAsRef<int>(0x0063C05C);
    static auto& medium_font_id = AddrAsRef<int>(0x0063C060);
    static auto& small_font_id = AddrAsRef<int>(0x0063C068);
    static auto& direct_input_disabled = AddrAsRef<bool>(0x005A4F88);
    static auto& default_player_weapon = AddrAsRef<String>(0x007C7600);
    static auto& active_cutscene = AddrAsRef<void*>(0x00645320);

    static auto& RfBeep = AddrAsRef<void(unsigned u1, unsigned u2, unsigned u3, float volume)>(0x00505560);
    static auto& GetFileExt = AddrAsRef<char *(const char *path)>(0x005143F0);
    static auto& SetNextLevelFilename = AddrAsRef<void(String::Pod level_filename, String::Pod save_filename)>(0x0045E2E0);
    static auto& DemoLoadLevel = AddrAsRef<void(const char *level_filename)>(0x004CC270);
    static auto& SetCursorVisible = AddrAsRef<void(bool visible)>(0x0051E680);
    static auto& CutsceneIsActive = AddrAsRef<bool()>(0x0045BE80);
    static const auto Timer__GetTimeLeftMs = reinterpret_cast<int(__thiscall*)(void* timer)>(0x004FA420);
    static auto& TimerGet = AddrAsRef<int(int mult)>(0x00504AB0);
    static auto& GeomClearCache = AddrAsRef<void()>(0x004F0B90);
    static auto& FileGetChecksum = AddrAsRef<int(const char* filename)>(0x00436630);

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
