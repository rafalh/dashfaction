#include "stdafx.h"
#include "misc.h"
#include "rf.h"
#include "version.h"
#include "utils.h"
#include "main.h"
#include "inline_asm.h"
#include <CallHook2.h>
#include <FunHook2.h>
#include <RegsPatch.h>
#include <rfproto.h>

namespace rf {
static auto &g_MenuVersionLabel = *(UiPanel*)0x0063C088;
auto& g_sound_enabled = AddrAsRef<bool>(0x017543D8);
}

constexpr int EGG_ANIM_ENTER_TIME = 2000;
constexpr int EGG_ANIM_LEAVE_TIME = 2000;
constexpr int EGG_ANIM_IDLE_TIME = 3000;

int g_VersionLabelX, g_VersionLabelWidth, g_VersionLabelHeight;
static const char g_szVersionInMenu[] = PRODUCT_NAME_VERSION;
int g_VersionClickCounter = 0;
int g_EggAnimStart;
bool g_bWin32Console = false;

using UiLabel_Create2_Type = void __fastcall(rf::UiPanel*, void*, rf::UiPanel*, int, int, int, int, const char*, int);
extern CallHook2<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_Hook;
void __fastcall UiLabel_Create2_VersionLabel(rf::UiPanel *pThis, void *edx, rf::UiPanel *pParent, int x, int y, int w, int h,
    const char *pszText, int FontId)
{
    x = g_VersionLabelX;
    w = g_VersionLabelWidth;
    h = g_VersionLabelHeight;
    UiLabel_Create2_VersionLabel_Hook.CallTarget(pThis, edx, pParent, x, y, w, h, pszText, FontId);
}
CallHook2<UiLabel_Create2_Type> UiLabel_Create2_VersionLabel_Hook{ 0x0044344D, UiLabel_Create2_VersionLabel };

FunHook2<void(const char**, const char**)> GetVersionStr_Hook{
    0x004B33F0,
    [](const char** version, const char** a2) {
        if (version)
            *version = g_szVersionInMenu;
        if (a2)
            *a2 = "";
        rf::GrGetTextWidth(&g_VersionLabelWidth, &g_VersionLabelHeight, g_szVersionInMenu, -1, rf::g_MediumFontId);

        g_VersionLabelX = 430 - g_VersionLabelWidth;
        g_VersionLabelWidth = g_VersionLabelWidth + 5;
        g_VersionLabelHeight = g_VersionLabelHeight + 2;
    }
};

FunHook2<int()> MenuUpdate_Hook{
    0x00434230,
    []() {
        int MenuId = MenuUpdate_Hook.CallTarget();
        if (MenuId == rf::GS_MP_LIMBO) // hide cursor when changing level - hackfixed in RF by chaning rendering logic
            rf::SetCursorVisible(false);
        else if (MenuId == rf::GS_MAIN_MENU)
            rf::SetCursorVisible(true);
        return MenuId;
    }};

ASM_FUNC(CrashFix_0055CE48,
// ecx - num, esi - source, ebx - dest
    ASM_I  shl   edi, 5
    ASM_I  lea   edx, [esp + 0x38 - 0x28]
    ASM_I  mov   eax, [eax + edi]
    ASM_I  test  eax, eax // check if pD3DTexture is NULL
    ASM_I  jz    ASM_LABEL(CrashFix_0055CE48_label1)
    ASM_I  push  0
    ASM_I  push  0
    ASM_I  push  edx
    ASM_I  push  0
    ASM_I  push  eax
    ASM_I  mov   ecx, 0x0055CE59
    ASM_I  jmp   ecx
    ASM_I  ASM_LABEL(CrashFix_0055CE48_label1) :
    ASM_I  mov   ecx, 0x0055CF23 // fail gr_lock
    ASM_I  jmp   ecx
)

CallHook2<void()> MenuMainProcessMouse_Hook{
    0x004437B9,
    []() {
        MenuMainProcessMouse_Hook.CallTarget();
        if (rf::MouseWasButtonPressed(0))
        {
            int x, y, z;
            rf::MouseGetPos(&x, &y, &z);
            rf::UiPanel *panels_to_check[1] = { &rf::g_MenuVersionLabel };
            int matched = rf::UiGetElementFromPos(x, y, panels_to_check, _countof(panels_to_check));
            if (matched == 0)
            {
                TRACE("Version clicked");
                ++g_VersionClickCounter;
                if (g_VersionClickCounter == 3)
                    g_EggAnimStart = GetTickCount();
            }
        }
    }
};

int LoadEasterEggImage()
{
#if 1 // from resources?
    HRSRC h_res = FindResourceA(g_hmodule, MAKEINTRESOURCEA(100), RT_RCDATA);
    if (!h_res)
    {
        ERR("FindResourceA failed");
        return -1;
    }
    HGLOBAL h_res_data = LoadResource(g_hmodule, h_res);
    if (!h_res_data)
    {
        ERR("LoadResource failed");
        return -1;
    }
    void *res_data = LockResource(h_res_data);
    if (!res_data)
    {
        ERR("LockResource failed");
        return -1;
    }

    constexpr int EASTER_EGG_SIZE = 128;

    int hbm = rf::BmCreateUserBmap(rf::BMPF_8888, EASTER_EGG_SIZE, EASTER_EGG_SIZE);

    rf::GrLockData lock_data;
    if (!rf::GrLock(hbm, 0, &lock_data, 1))
        return -1;

    rf::BmConvertFormat(lock_data.pBits, (rf::BmPixelFormat)lock_data.PixelFormat, res_data, rf::BMPF_8888,
        EASTER_EGG_SIZE * EASTER_EGG_SIZE);
    rf::GrUnlock(&lock_data);

    return hbm;
#else
    return BmLoad("DF_pony.tga", -1, 0);
#endif
}

CallHook2<void()> MenuMainRender_Hook{
    0x00443802,
    []() {
        MenuMainRender_Hook.CallTarget();
        if (g_VersionClickCounter >= 3) {
            static int img = LoadEasterEggImage(); // data.vpp
            if (img == -1)
                return;
            int w, h;
            rf::BmGetBitmapSize(img, &w, &h);
            int anim_delta_time = GetTickCount() - g_EggAnimStart;
            int pos_x = (rf::GrGetMaxWidth() - w) / 2;
            int pos_y = rf::GrGetMaxHeight() - h;
            if (anim_delta_time < EGG_ANIM_ENTER_TIME)
                pos_y += h - (int)(sinf(anim_delta_time / (float)EGG_ANIM_ENTER_TIME * (float)M_PI / 2.0f) * h);
            else if (anim_delta_time > EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME) {
                int leave_delta = anim_delta_time - (EGG_ANIM_ENTER_TIME + EGG_ANIM_IDLE_TIME);
                pos_y += (int)((1.0f - cosf(leave_delta / (float)EGG_ANIM_LEAVE_TIME * (float)M_PI / 2.0f)) * h);
                if (leave_delta > EGG_ANIM_LEAVE_TIME)
                    g_VersionClickCounter = 0;
            }
            rf::GrDrawImage(img, pos_x, pos_y, rf::g_GrBitmapMaterial);
        }
    }
};

void SetPlaySoundEventsVolumeScale(float volume_scale)
{
    volume_scale = clamp(volume_scale, 0.0f, 1.0f);
    uintptr_t Offsets[] = {
        // Play Sound event
        0x004BA4D8, 0x004BA515, 0x004BA71C, 0x004BA759, 0x004BA609, 0x004BA5F2, 0x004BA63F,
    };
    for (int i = 0; i < COUNTOF(Offsets); ++i)
        WriteMemFloat(Offsets[i] + 1, volume_scale);
}

CallHook2<void(int, rf::Vector3*, float*, float*, float)> SndConvertVolume3D_AmbientSound_Hook{
    0x00505F93,
    [](int game_snd_id, rf::Vector3* sound_pos, float* pan_out, float* volume_out, float volume_in) {
        SndConvertVolume3D_AmbientSound_Hook.CallTarget(game_snd_id, sound_pos, pan_out, volume_out, volume_in);
        *volume_out *= g_game_config.levelSoundVolume;
    }
};

FunHook2<void()> MouseUpdateDirectInput_Hook{
    0x0051DEB0,
    []() {
        MouseUpdateDirectInput_Hook.CallTarget();

        // center cursor
        POINT pt{ rf::GrGetMaxWidth() / 2, rf::GrGetMaxHeight() / 2 };
        ClientToScreen(rf::g_hWnd, &pt);
        SetCursorPos(pt.x, pt.y);
    }
};

bool IsHoldingAssaultRifle()
{
    static auto &assault_rifle_cls_id = AddrAsRef<int>(0x00872470);
    rf::EntityObj *entity = rf::EntityGetFromHandle(rf::g_pLocalPlayer->hEntity);
    return entity && entity->WeaponInfo.WeaponClsId == assault_rifle_cls_id;
}

FunHook2<void(rf::Player*, bool, bool)> PlayerLocalFireControl_Hook{
    0x004A4E80,
    [](rf::Player* player, bool secondary, bool was_pressed) {
        if (g_game_config.swapAssaultRifleControls && IsHoldingAssaultRifle())
            secondary = !secondary;
        PlayerLocalFireControl_Hook.CallTarget(player, secondary, was_pressed);
    }
};

extern CallHook2<char(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_Hook1;
char IsEntityCtrlActive_New(rf::ControlConfig* control_config, rf::GameCtrl game_ctrl, bool* was_pressed)
{
    if (g_game_config.swapAssaultRifleControls && IsHoldingAssaultRifle()) {
        if (game_ctrl == rf::GC_PRIMARY_ATTACK)
            game_ctrl = rf::GC_SECONDARY_ATTACK;
        else if (game_ctrl == rf::GC_SECONDARY_ATTACK)
            game_ctrl = rf::GC_PRIMARY_ATTACK;
    }
    return IsEntityCtrlActive_Hook1.CallTarget(control_config, game_ctrl, was_pressed);
}
CallHook2<char(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_Hook1{ 0x00430E65, IsEntityCtrlActive_New };
CallHook2<char(rf::ControlConfig*, rf::GameCtrl, bool*)> IsEntityCtrlActive_Hook2{ 0x00430EF7, IsEntityCtrlActive_New };

void DcfSwapAssaultRifleControls()
{
    if (rf::g_bDcRun) {
        g_game_config.swapAssaultRifleControls = !g_game_config.swapAssaultRifleControls;
        g_game_config.save();
        rf::DcPrintf("Swap assault rifle controls: %s", g_game_config.swapAssaultRifleControls ? "enabled" : "disabled");
    }
}

#if SERVER_WIN32_CONSOLE

static const auto KeyProcessEvent = (void(*)(int ScanCode, int bKeyDown, int DeltaT))0x0051E6C0;

void ResetConsoleCursorColumn(bool clear)
{
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ScrBufInfo;
    GetConsoleScreenBufferInfo(hOutput, &ScrBufInfo);
    if (ScrBufInfo.dwCursorPosition.X == 0)
        return;
    COORD NewPos = ScrBufInfo.dwCursorPosition;
    NewPos.X = 0;
    SetConsoleCursorPosition(hOutput, NewPos);
    if (clear) {
        for (int i = 0; i < ScrBufInfo.dwCursorPosition.X; ++i)
            WriteConsoleA(hOutput, " ", 1, NULL, NULL);
        SetConsoleCursorPosition(hOutput, NewPos);
    }
}

void PrintCmdInputLine()
{
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ScrBufInfo;
    GetConsoleScreenBufferInfo(hOutput, &ScrBufInfo);
    WriteConsoleA(hOutput, "] ", 2, NULL, NULL);
    unsigned Offset = std::max(0, (int)g_cchDcCmdLineLen - ScrBufInfo.dwSize.X + 3);
    WriteConsoleA(hOutput, g_szDcCmdLine + Offset, g_cchDcCmdLineLen - Offset, NULL, NULL);
}

BOOL WINAPI ConsoleCtrlHandler(DWORD fdwCtrlType)
{
    INFO("Quiting after Console CTRL");
    static auto &Close = *(int32_t*)0x01B0D758;
    Close = 1;
    return TRUE;
}

void InputThreadProc()
{
    while (true) {
        INPUT_RECORD InputRecord;
        DWORD NumRead = 0;
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &InputRecord, 1, &NumRead);
    }
}

CallHook2<void()> OsInitWindow_Server_Hook{ 0x004B27C5,
    []() {
        AllocConsole();
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

        //std::thread InputThread(InputThreadProc);
        //InputThread.detach();
    }
};

FunHook2<void(const char*, const int*)> DcPrint_Hook{
    reinterpret_cast<uintptr_t>(DcPrint),
    [](const char* pszText, const int* pColor) {
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        constexpr WORD RedAttr = FOREGROUND_RED | FOREGROUND_INTENSITY;
        constexpr WORD BlueAttr = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD WhiteAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD GrayAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        WORD CurrentAttr = 0;

        ResetConsoleCursorColumn(true);

        const char *Ptr = pszText;
        while (*Ptr) {
            std::string Color;
            if (Ptr[0] == '[' && Ptr[1] == '$') {
                const char *ColorEndPtr = strchr(Ptr + 2, ']');
                if (ColorEndPtr) {
                    Color.assign(Ptr + 2, ColorEndPtr - Ptr - 2);
                    Ptr = ColorEndPtr + 1;
                }
            }

            const char *EndPtr = strstr(Ptr, "[$");
            if (!EndPtr)
                EndPtr = Ptr + strlen(Ptr);

            WORD Attr;
            if (Color == "Red")
                Attr = RedAttr;
            else if (Color == "Blue")
                Attr = BlueAttr;
            else if (Color == "White")
                Attr = WhiteAttr;
            else {
                if (!Color.empty())
                    ERR("unknown color %s", Color.c_str());
                Attr = GrayAttr;
            }

            if (CurrentAttr != Attr) {
                CurrentAttr = Attr;
                SetConsoleTextAttribute(hOutput, Attr);
            }
            
            DWORD NumChars = EndPtr - Ptr;
            WriteFile(hOutput, Ptr, NumChars, NULL, NULL);
            Ptr = EndPtr;
        }

        if (Ptr > pszText && Ptr[-1] != '\n')
            WriteFile(hOutput, "\n", 1, NULL, NULL);

        if (CurrentAttr != GrayAttr)
            SetConsoleTextAttribute(hOutput, GrayAttr);

        //PrintCmdInputLine();
    }
};

CallHook2<void()> DcPutChar_NewLine_Hook{
    0x0050A081,
    [] {
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(hOutput, "\r\n", 2, NULL, NULL);
    }
};

FunHook2<void()> DcDrawServerConsole_Hook{
    0x0050A770,
    []() {
        static char szPrevCmdLine[1024];
        HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        if (strncmp(g_szDcCmdLine, szPrevCmdLine, _countof(szPrevCmdLine)) != 0) {
            ResetConsoleCursorColumn(true);
            PrintCmdInputLine();
            strncpy(szPrevCmdLine, g_szDcCmdLine, _countof(szPrevCmdLine));
        }
    }
};

FunHook2<int()> KeyGetFromQueue_Hook{
    0x0051F000,
    []() {
        if (!rf::g_bDedicatedServer)
            return KeyGetFromQueue_Hook.CallTarget();

        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD InputRecord;
        DWORD NumRead = 0;
        while (false) {
            if (!PeekConsoleInput(hInput, &InputRecord, 1, &NumRead) || NumRead == 0)
                break;
            if (!ReadConsoleInput(hInput, &InputRecord, 1, &NumRead) || NumRead == 0)
                break;
            if (InputRecord.EventType == KEY_EVENT)
                KeyProcessEvent(InputRecord.Event.KeyEvent.wVirtualScanCode, InputRecord.Event.KeyEvent.bKeyDown, 0);
        }

        return KeyGetFromQueue_Hook.CallTarget();
    }
};

#endif // SERVER_WIN32_CONSOLE

bool EntityIsReloading_SwitchWeapon_New(rf::EntityObj *entity)
{
    if (rf::EntityIsReloading(entity))
        return true;

    int weapon_cls_id = entity->WeaponInfo.WeaponClsId;
    if (weapon_cls_id >= 0) {
        rf::WeaponClass* weapon_cls = &rf::g_pWeaponClasses[weapon_cls_id];
        if (entity->WeaponInfo.WeaponsAmmo[weapon_cls_id] == 0 && entity->WeaponInfo.ClipAmmo[weapon_cls->AmmoType] > 0)
            return true;
    }
    return false;
}

FunHook2<int(void*, void*)> GeomCachePrepareRoom_Hook{
    0x004F0C00,
    [](void* geom, void* room) {
        int ret = GeomCachePrepareRoom_Hook.CallTarget(geom, room);
        char** ppRoomGeom = (char**)((char*)room + 4);
        char* pRoomGeom = *ppRoomGeom;
        if (ret == 0 && pRoomGeom)
        {
            uint32_t *pRoomVertNum = (uint32_t*)(pRoomGeom + 4);
            if (*pRoomVertNum > 8000)
            {
                static int Once = 0;
                if (!(Once++))
                    WARN("Not rendering room with %u vertices!", *pRoomVertNum);
                *ppRoomGeom = NULL;
                return -1;
            }
        }
        return ret;
    }
};

struct ServerListEntry
{
    char name[32];
    char level_name[32];
    char mod_name[16];
    int game_type;
    rf::NwAddr addr;
    char current_players;
    char max_players;
    int16_t ping;
    int field_60;
    char field_64;
    int Flags;
};
static_assert(sizeof(ServerListEntry) == 0x6C, "invalid size");

FunHook2<int (const int&, const int&)> ServerListCmpFunc_Hook{
    0x0044A6D0,
    [](const int& index1, const int& index2) {
        auto ServerList = AddrAsRef<ServerListEntry*>(0x0063F62C);
        bool has_ping1 = ServerList[index1].ping >= 0;
        bool has_ping2 = ServerList[index2].ping >= 0;
        if (has_ping1 != has_ping2)
            return has_ping1 ? -1 : 1;
        else
            return ServerListCmpFunc_Hook.CallTarget(index1, index2);
    }
};

constexpr int CHAT_MSG_MAX_LEN = 224;

FunHook2<void(uint16_t)> ChatSayAddChar_Hook{
    0x00444740,
    [](uint16_t key) {
        if (key)
            ChatSayAddChar_Hook.CallTarget(key);
    }
};

FunHook2<void(const char*, bool)> ChatSayAccept_Hook{
    0x00444440,
    [](const char* msg, bool is_team_msg) {
        std::string msg_str{msg};
        if (msg_str.size() > CHAT_MSG_MAX_LEN)
            msg_str.resize(CHAT_MSG_MAX_LEN);
        ChatSayAccept_Hook.CallTarget(msg_str.c_str(), is_team_msg);
    }
};

constexpr uint8_t TRIGGER_CLIENT_SIDE = 0x2;
constexpr uint8_t TRIGGER_SOLO        = 0x4;
constexpr uint8_t TRIGGER_TELEPORT    = 0x8;

void SendTriggerActivatePacket(rf::Player *player, rf::TriggerObj *trigger, int32_t h_entity)
{
    rfTriggerActivate packet;
    packet.type = RF_TRIGGER_ACTIVATE;
    packet.size = sizeof(packet) - sizeof(rfPacketHeader);
    packet.uid = trigger->_Super.Uid;
    packet.entity_handle = h_entity;
    rf::NwSendReliablePacket(player, reinterpret_cast<uint8_t*>(&packet), sizeof(packet), 0);
}

FunHook2<void(rf::TriggerObj*, int32_t, bool)> TriggerActivate_Hook{
    0x004C0220,
    [](rf::TriggerObj *trigger, int32_t h_entity, bool skip_movers) {
        // Check team
        auto player = rf::GetPlayerFromEntityHandle(h_entity);
        auto trigger_name = trigger->_Super.strName.CStr();
        if (player && trigger->Team != -1 && trigger->Team != player->bBlueTeam) {
            //rf::DcPrintf("Trigger team does not match: %d vs %d (%s)", trigger->Team, Player->bBlueTeam, trigger_name);
            return;
        }

        // Check if this is Solo or Teleport trigger (REDPF feature)
        uint8_t ext_flags = trigger_name[0] == '\xAB' ? trigger_name[1] : 0;
        bool is_solo_trigger = (ext_flags & (TRIGGER_SOLO | TRIGGER_TELEPORT)) != 0;
        if (rf::g_bNetworkGame && rf::g_bLocalNetworkGame && is_solo_trigger && player) {
            //rf::DcPrintf("Solo/Teleport trigger activated %s", trigger_name);
            SendTriggerActivatePacket(player, trigger, h_entity);
            return;
        }

        // Normal activation
        //rf::DcPrintf("trigger normal activation %s %d", trigger_name, ext_flags);
        TriggerActivate_Hook.CallTarget(trigger, h_entity, skip_movers);
    }
};

extern "C" bool IsClientSideTrigger(rf::TriggerObj *trigger)
{
    auto trigger_name = trigger->_Super.strName.CStr();
    uint8_t ext_flags = trigger_name[0] == '\xAB' ? trigger_name[1] : 0;
    return (ext_flags & TRIGGER_CLIENT_SIDE) != 0;
}

ASM_FUNC(TriggerCheckActivation_Patch_004BFCCD,
    ASM_I  cmp  ebp, eax
    ASM_I  jz   ASM_LABEL(TriggerCheckActivation_Patch_Activate)
    ASM_I  push [esp+0x1C+0x4] // Trigger
    ASM_I  call ASM_SYM(IsClientSideTrigger)
    ASM_I  add  esp, 4
    ASM_I  jnz  ASM_LABEL(TriggerCheckActivation_Patch_Activate)

    ASM_I  pop  edi
    ASM_I  pop  esi
    ASM_I  pop  ebp
    ASM_I  push 0x004BFCD4
    ASM_I  ret

    ASM_I  ASM_LABEL(TriggerCheckActivation_Patch_Activate) :
    ASM_I  push 0x004BFCDB
    ASM_I  ret
)

RegsPatch RflLoadInternal_CheckRestoreStatus_Patch{
    0x00461195,
    [](X86Regs& regs) {
        // check if SaveRestoreLoadAll is successful
        if (regs.eax) return;
        // check if this is auto-load when changing level
        const char *SaveFilename = reinterpret_cast<const char*>(regs.edi);
        if (!strcmp(SaveFilename, "auto.svl")) return;
        // manual load failed
        ERR("Restoring game state failed");
        char *ErrorInfo = *reinterpret_cast<char**>(regs.esp + 0x2B0 + 0xC);
        strcpy(ErrorInfo, "Save file is corrupted");
        // return to RflLoadInternal failure path
        regs.eip = 0x004608CC;
    }
};

static int g_cutscene_bg_sound_sig = -1;

FunHook2<void(bool)> MenuInGameUpdateCutscene_Hook{
    0x0045B5E0,
    [](bool dlg_open) {
        bool skip_cutscene = false;
        rf::IsEntityCtrlActive(&rf::g_pLocalPlayer->Config.Controls, rf::GC_JUMP, &skip_cutscene);

        if (!skip_cutscene) {
            MenuInGameUpdateCutscene_Hook.CallTarget(dlg_open);
            
            rf::GrSetColor(255, 255, 255, 255);
            rf::GrDrawAlignedText(rf::GR_ALIGN_CENTER, rf::GrGetMaxWidth() / 2, rf::GrGetMaxHeight() - 30,
                "Press JUMP key to skip the cutscene", -1, rf::g_GrTextMaterial);
        }
        else {
            auto CutsceneIsActive = AddrAsRef<bool()>(0x0045BE80);
            auto TimerAddDeltaTime = AddrAsRef<int(int DeltaMs)>(0x004FA2D0);
            auto SndStop = AddrAsRef<char(int Sig)>(0x005442B0);
            auto Timer__GetTimeLeftMs = AddrAsRef<int __thiscall(void *Timer)>(0x004FA420);
            auto DestroyAllPausedSounds = AddrAsRef<void()>(0x005059F0);
            auto SetAllPlayingSoundsPaused = AddrAsRef<void(bool Paused)>(0x00505C70);
            
            auto &timer_base = AddrAsRef<int64_t>(0x01751BF8);
            auto &timer_freq = AddrAsRef<int32_t>(0x01751C04);
            auto &frame_time = AddrAsRef<float>(0x005A4014);
            auto &active_cutscene = AddrAsRef<void*>(0x00645320);
            auto &current_shot_idx = StructFieldRef<int>(active_cutscene, 0x808);
            void *current_shot_timer = reinterpret_cast<char*>(active_cutscene) + 0x810;
            auto &num_shots = StructFieldRef<int>(active_cutscene, 4);

            if (g_cutscene_bg_sound_sig != -1) {
                SndStop(g_cutscene_bg_sound_sig);
                g_cutscene_bg_sound_sig = -1;
            }

            SetAllPlayingSoundsPaused(true);
            DestroyAllPausedSounds();
            rf::g_sound_enabled = false;

            while (CutsceneIsActive()) {
                int shot_time_left_ms = Timer__GetTimeLeftMs(current_shot_timer);
                
                if (current_shot_idx == num_shots - 1)
                {
                    // run last half second with a speed of 10 FPS so all events get properly processed before
                    // going back to normal gameplay
                    if (shot_time_left_ms > 500)
                        shot_time_left_ms -= 500;
                    else
                        shot_time_left_ms = std::min(shot_time_left_ms, 100);
                }
                TimerAddDeltaTime(shot_time_left_ms);
                frame_time = shot_time_left_ms / 1000.0f;
                timer_base -= static_cast<int64_t>(shot_time_left_ms) * timer_freq / 1000;
                MenuInGameUpdateCutscene_Hook.CallTarget(dlg_open);
            }

            rf::g_sound_enabled = true;
        }
    }
};

CallHook2<int()> PlayHardcodedBackgroundMusicForCutscene_Hook{
    0x0045BB85,
    []() {
        g_cutscene_bg_sound_sig = PlayHardcodedBackgroundMusicForCutscene_Hook.CallTarget();
        return g_cutscene_bg_sound_sig;
    }
};

auto CanSave = AddrAsRef<bool()>(0x004B61A0);

FunHook2<void()> DoQuickSave_Hook{
    0x004B5E20,
    []() {
        if (CanSave())
            DoQuickSave_Hook.CallTarget();
    }
};

auto MultiIsConnected = AddrAsRef<bool()>(0x0044AD70);
auto GameSeqPushState = AddrAsRef<int(int state, bool update_parent_state, bool parent_dlg_open)>(0x00434410);

bool g_jump_to_multi_server_list = false;

FunHook2<void(int, int)> GameEnterState_Hook{
    0x004B1AC0,
    [](int state, int old_state) {
        GameEnterState_Hook.CallTarget(state, old_state);
        if (state == rf::GS_MAIN_MENU && old_state == rf::GS_EXIT_GAME && g_jump_to_multi_server_list) {
            rf::g_sound_enabled = false;
            GameSeqPushState(rf::GS_MP_MENU, false, false);
        }
        if (state == rf::GS_MP_MENU && g_jump_to_multi_server_list) {
            GameSeqPushState(rf::GS_MP_SERVER_LIST_MENU, false, false);
            g_jump_to_multi_server_list = false;
            rf::g_sound_enabled = true;
        }
    }
};

FunHook2<void()> MultiAfterPlayersPackets_Hook{
    0x00482080,
    []() {
        MultiAfterPlayersPackets_Hook.CallTarget();
        g_jump_to_multi_server_list = true;
    }
};

void MiscInit()
{
    // Console init string
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    // Version in Main Menu
    UiLabel_Create2_VersionLabel_Hook.Install();
    GetVersionStr_Hook.Install();

    // Window title (client and server)
    WriteMemPtr(0x004B2790, PRODUCT_NAME);
    WriteMemPtr(0x004B27A4, PRODUCT_NAME);

    // Console background color
    WriteMemUInt32(0x005098D1, CONSOLE_BG_A); // Alpha
    WriteMemUInt8(0x005098D6, CONSOLE_BG_B); // Blue
    WriteMemUInt8(0x005098D8, CONSOLE_BG_G); // Green
    WriteMemUInt8(0x005098DA, CONSOLE_BG_R); // Red

#ifdef NO_CD_FIX
    // No-CD fix
    WriteMemUInt8(0x004B31B6, ASM_SHORT_JMP_REL);
#endif // NO_CD_FIX

#ifdef NO_INTRO
    // Disable thqlogo.bik
    if (g_game_config.fastStart) {
        WriteMemUInt8(0x004B208A, ASM_SHORT_JMP_REL);
        WriteMemUInt8(0x004B24FD, ASM_SHORT_JMP_REL);
    }
#endif // NO_INTRO

    // Sound loop fix
    WriteMemUInt8(0x00505D08, 0x00505D5B - (0x00505D07 + 0x2));

    // Set initial FPS limit
    WriteMemFloat(0x005094CA, 1.0f / g_game_config.maxFps);

    // Crash-fix... (probably argument for function is invalid); Page Heap is needed
    WriteMemUInt32(0x0056A28C + 1, 0);

    // Crash-fix in case texture has not been created (this happens if GrReadBackbuffer fails)
    AsmWritter(0x0055CE47).jmpLong(CrashFix_0055CE48);

    // Dont overwrite player name and prefered weapons when loading saved game
    AsmWritter(0x004B4D99, 0x004B4DA5).nop();
    AsmWritter(0x004B4E0A, 0x004B4E22).nop();

    // Dont filter levels for DM and TeamDM
    WriteMemUInt8(0x005995B0, 0);
    WriteMemUInt8(0x005995B8, 0);

#if DIRECTINPUT_SUPPORT
    rf::g_bDirectInputDisabled = 0;
#endif

#if 1
    // Buffer overflows in RflReadStaticGeometry
    // Note: Buffer size is 1024 but opcode allows only 1 byte size
    //       What is more important BmLoad copies texture name to 32 bytes long buffers
    WriteMemInt8(0x004ED612 + 1, 32);
    WriteMemInt8(0x004ED66E + 1, 32);
    WriteMemInt8(0x004ED72E + 1, 32);
    WriteMemInt8(0x004EDB02 + 1, 32);
#endif

#if 1 // Version Easter Egg
    MenuMainProcessMouse_Hook.Install();
    MenuMainRender_Hook.Install();
#endif

    // Fix console rendering when changing level
    AsmWritter(0x0047C490).ret();
    AsmWritter(0x0047C4AA).ret();
    WriteMemUInt8(0x004B2E15, ASM_NOP, 2);
    MenuUpdate_Hook.Install();

    // Increase damage for kill command in Single Player
    WriteMemFloat(0x004A4DF5 + 1, 100000.0f);
    
    // Fix keyboard layout
    uint8_t kbd_layout = 0;
    if (MapVirtualKeyA(0x10, MAPVK_VSC_TO_VK) == 'A')
        kbd_layout = 2; // AZERTY
    else if (MapVirtualKeyA(0x15, MAPVK_VSC_TO_VK) == 'Z')
        kbd_layout = 3; // QWERTZ
    INFO("Keyboard layout: %u", kbd_layout);
    WriteMemUInt8(0x004B14B4 + 1, kbd_layout);

    // Level sounds
    SetPlaySoundEventsVolumeScale(g_game_config.levelSoundVolume);
    SndConvertVolume3D_AmbientSound_Hook.Install();

    // hook MouseUpdateDirectInput
    MouseUpdateDirectInput_Hook.Install();

    // Chat color alpha
    AsmWritter(0x00477490, 0x004774A4).mov(AsmRegs::EAX, 0x30); // chatbox border
    AsmWritter(0x00477528, 0x00477535).mov(AsmRegs::EBX, 0x40); // chatbox background
    AsmWritter(0x00478E00, 0x00478E14).mov(AsmRegs::EAX, 0x30); // chat input border
    AsmWritter(0x00478E91, 0x00478E9E).mov(AsmRegs::EBX, 0x40); // chat input background

    // Show enemy bullets (FIXME: add config)
    if (g_game_config.showEnemyBullets)
        WriteMemUInt8(0x0042669C, ASM_SHORT_JMP_REL);

    // Swap Assault Rifle fire controls
    PlayerLocalFireControl_Hook.Install();
    IsEntityCtrlActive_Hook1.Install();
    IsEntityCtrlActive_Hook2.Install();
    DC_REGISTER_CMD(swap_assault_rifle_controls, "Swap Assault Rifle controls", DcfSwapAssaultRifleControls);

    // Fix crash in shadows rendering
    WriteMemUInt8(0x0054A3C0 + 2, 16);

    // Fix crash in geometry rendering
    GeomCachePrepareRoom_Hook.Install();

    // Remove Sleep calls in TimerInit
    AsmWritter(0x00504A67, 0x00504A82).nop();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    WriteMemUInt8(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    WriteMemUInt8(0x0047039A, 0x74); // invert jump condition: jnz -> jz

    // Put not responding servers at the bottom of server list
    ServerListCmpFunc_Hook.Install();

    // Disable broken optimization of segment vs geometry collision test
    // Fixes hitting objects if mover is in the line of the shot
    AsmWritter(0x00499055).jmpLong(0x004990B4);

    // Disable Flamethower debug sphere drawing (optimization)
    // It is not visible in game because other things are drawn over it
    AsmWritter(0x0041AE47, 0x0041AE4C).nop();

    // Fix game beeping every frame if chat input buffer is full
    ChatSayAddChar_Hook.Install();

    // Change chat input limit to 224 (RF can support 255 safely but PF kicks if message is longer than 224)
    WriteMemInt32(0x0044474A + 1, CHAT_MSG_MAX_LEN);

    // Add chat message limit for say/teamsay commands
    ChatSayAccept_Hook.Install();

    // Preserve password case when processing rcon_request command
    WriteMemInt8(0x0046C85A + 1, 1);

    // Solo/Teleport triggers handling + filtering by team ID
    TriggerActivate_Hook.Install();

    // Client-side trigger flag handling
    AsmWritter(0x004BFCCD, 0x004BFCD4).jmpLong(TriggerCheckActivation_Patch_004BFCCD);

    // Fix crash when loading savegame with missing player entity data
    AsmWritter(0x004B4B47).jmpNear(0x004B4B7B);

    // Add checking if restoring game state from save file failed during level loading
    RflLoadInternal_CheckRestoreStatus_Patch.Install();

    // Fix creating corrupted saves if cutscene starts in the same frame as quick save button is pressed
    DoQuickSave_Hook.Install();

    // Support skipping cutscenes
    MenuInGameUpdateCutscene_Hook.Install();
    PlayHardcodedBackgroundMusicForCutscene_Hook.Install();

    // Open server list menu instead of main menu when leaving multiplayer game
    GameEnterState_Hook.Install();
    MultiAfterPlayersPackets_Hook.Install();

#if 0
    // Fix weapon switch glitch when reloading (should be used on Match Mode)
    AsmWritter(0x004A4B4B).callLong(EntityIsReloading_SwitchWeapon_New);
    AsmWritter(0x004A4B77).callLong(EntityIsReloading_SwitchWeapon_New);
#endif

#if SERVER_WIN32_CONSOLE // win32 console
    g_bWin32Console = stristr(GetCommandLineA(), "-win32-console") != nullptr;
    if (g_bWin32Console) {
        OsInitWindow_Server_Hook.Install();
        //AsmWritter(0x0050A770).ret(); // null DcDrawServerConsole
        DcPrint_Hook.Install();
        DcDrawServerConsole_Hook.Install();
        KeyGetFromQueue_Hook.Install();
        DcPutChar_NewLine_Hook.Install();
    }
#endif
}

void MiscCleanup()
{
#if SERVER_WIN32_CONSOLE // win32 console
    if (g_bWin32Console)
        FreeConsole();
#endif
}
