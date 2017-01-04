#ifndef RF_H_INCLUDED
#define RF_H_INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef BOOL WINBOOL;

#include <d3d8.h>

//#pragma pack(push, 1)

typedef struct _CVector3
{
	float x, y, z;
} CVector3;

typedef struct _CMatrix3
{
    CVector3 rows[3];
} CMatrix3;

/* String */

typedef struct _CString
{
    uint32_t cch;
    char *psz;
} CString;

typedef char *(*PFN_STRING_ALLOC)(unsigned cbSize);
static PFN_STRING_ALLOC const StringAlloc = (PFN_STRING_ALLOC)0x004FF300;

typedef void (*PFN_STRING_FREE)(void *pData);
static PFN_STRING_FREE const StringFree = (PFN_STRING_FREE)0x004FF3A0;

/* Console */

typedef unsigned (*PFN_CONSOLE_WRITE)(const char *pszText, uint32_t *pColor);
static const PFN_CONSOLE_WRITE RfConsoleWrite = (PFN_CONSOLE_WRITE)0x00509EC0;

typedef unsigned (*PFN_CONSOLE_PRINTF)(const char *pszFormat, ...);
static const PFN_CONSOLE_PRINTF RfConsolePrintf = (PFN_CONSOLE_PRINTF)0x0050B9F0;

/* Commands */

typedef struct _CCmd
{
    const char *pszCmd;
    const char *pszDescr;
    void (*pfnHandler)(void);
} CCmd;

#define MAX_COMMANDS_COUNT 30
static CCmd ** const g_ppCommands = (CCmd**)0x01775530;
static uint32_t * const g_pcCommands = (uint32_t*)0x0177567C;

static uint32_t * const g_pbCmdRun = (uint32_t*)0x01775110;
static uint32_t * const g_pbCmdHelp = (uint32_t*)0x01775224;
static uint32_t * const g_piCmdArgType = (uint32_t*)0x01774D04;
static char * const g_pszCmdArg = (char*)0x0175462C;
static int * const g_piCmdArg = (int*)0x01775220;
static float * const g_pfCmdArg = (float*)0x01754628;

#define CMD_ARG_NONE  0x0001
#define CMD_ARG_STR   0x0002
#define CMD_ARG_INT   0x0008
#define CMD_ARG_FLOAT 0x0010
#define CMD_ARG_HEX   0x0020
#define CMD_ARG_TRUE  0x0040
#define CMD_ARG_FALSE 0x0080
#define CMD_ARG_PLUS  0x0100
#define CMD_ARG_MINUS 0x0200
#define CMD_ARG_COMMA 0x0400

typedef void (*PFN_CMD_GET_NEXT_ARG)(int Type, int bUnknown);
static const PFN_CMD_GET_NEXT_ARG RfCmdGetNextArg = (PFN_CMD_GET_NEXT_ARG)0x0050AED0;

/* Banlist */

typedef struct _BANLIST_ENTRY
{
    char szIp[24];
    struct _BANLIST_ENTRY *pNext;
    struct _BANLIST_ENTRY *pPrev;
} BANLIST_ENTRY;

static BANLIST_ENTRY ** const g_ppBanlistFirstEntry = (BANLIST_ENTRY**)0x0064EC20;
static BANLIST_ENTRY ** const g_ppBanlistLastEntry = (BANLIST_ENTRY**)0x0064EC24;
static BANLIST_ENTRY * const g_pBanlistNullEntry = (BANLIST_ENTRY*)0x0064EC08;

/* Graphics */

static IDirect3D8 ** const g_ppDirect3D = (IDirect3D8**)0x01CFCBE0;
static IDirect3DDevice8 ** const g_ppGrDevice = (IDirect3DDevice8**)0x01CFCBE4;
static uint32_t * const g_pGrGammaRamp = (uint32_t*)0x017C7C68;
static uint32_t * const g_pAdapterIdx = (uint32_t*)0x01CFCC34;
constexpr CVector3 *g_pGrScaleVec = (CVector3*)0x01818B48;
constexpr CMatrix3 *g_GrViewMatrix = (CMatrix3*)0x018186C8;

typedef void (*PFN_GR_SET_COLOR_RGB)(unsigned r, unsigned g, unsigned b, unsigned a);
static const PFN_GR_SET_COLOR_RGB GrSetColorRgb = (PFN_GR_SET_COLOR_RGB)0x0050CF80;

typedef void (*PFN_GR_SET_COLOR_PTR)(uint32_t *pColor);
static const PFN_GR_SET_COLOR_PTR GrSetColorPtr = (PFN_GR_SET_COLOR_PTR)0x0050D000;

typedef void (*PFN_GR_DRAW_TEXT)(unsigned x, unsigned y, const char *pszText, int Font, unsigned Unknown);
static const PFN_GR_DRAW_TEXT GrDrawText = (PFN_GR_DRAW_TEXT)0x0051FEB0;

typedef void (*PFN_GR_DRAW_ALIGNED_TEXT)(unsigned Align, unsigned x, unsigned y, const char *pszText, int Font, unsigned Unknown);
static const PFN_GR_DRAW_ALIGNED_TEXT GrDrawAlignedText = (PFN_GR_DRAW_ALIGNED_TEXT)0x0051FE50;

typedef void (*PFN_GR_DRAW_RECT)(unsigned x, unsigned y, unsigned cx, unsigned cy, unsigned Unknown);
static const PFN_GR_DRAW_RECT GrDrawRect = (PFN_GR_DRAW_RECT)0x0050DBE0;

typedef unsigned (*PFN_GR_GET_FONT_HEIGHT)(int Font);
static const PFN_GR_GET_FONT_HEIGHT GrGetFontHeight = (PFN_GR_GET_FONT_HEIGHT)0x0051F4D0;

typedef CString *(*PFN_GR_FIT_TEXT)(CString *pstrDest, CString Str, int cxMax);
static const PFN_GR_FIT_TEXT GrFitText = (PFN_GR_FIT_TEXT)0x00471EC0;

typedef int (*PFN_GR_LOAD_TEXTURE)(const char *pszFilename, int a2, BOOL a3);
static const PFN_GR_LOAD_TEXTURE GrLoadTexture = (PFN_GR_LOAD_TEXTURE)0x0050F6A0;

typedef int (*PFN_GR_DRAW_IMAGE)(int Texture, int x, int y, int a4);
static const PFN_GR_DRAW_IMAGE GrDrawImage = (PFN_GR_DRAW_IMAGE)0x0050D2A0;

typedef int(*PFN_GR_READ_BACK_BUFFER)(int x, int y, int Width, int Height, void *pBuffer);
static const PFN_GR_READ_BACK_BUFFER GrReadBackBuffer = (PFN_GR_READ_BACK_BUFFER)0x0050DFF0;

/* GUI */

typedef void (*PFN_MSG_BOX)(const char *pszTitle, const char *pszText, void(*pfnCallback)(void), BOOL bInput);
static const PFN_MSG_BOX RfMsgBox = (PFN_MSG_BOX)0x004560B0;

typedef void (*PFN_CREATE_DIALOG)(const char *pszTitle, const char *pszText, unsigned cButtons, const char **ppszBtnTitles, void **ppfnCallbacks, unsigned Unknown1, unsigned Unknown2);
static const PFN_CREATE_DIALOG RfCreateDialog = (PFN_CREATE_DIALOG)0x004562A0;

/* VFS */

struct _PACKFILE_ENTRY;

typedef struct _PACKFILE
{
    char szName[32];
    char szPath[128];
    uint32_t field_A0;
    uint32_t cFiles;
    struct _PACKFILE_ENTRY *pFileList;
    uint32_t cbSize;
} PACKFILE;

typedef struct _PACKFILE_ENTRY
{
    uint32_t dwNameChecksum;
    char *pszFileName;
    uint32_t OffsetInBlocks;
    uint32_t cbFileSize;
    PACKFILE *pArchive;
    FILE *pRawFile;
} PACKFILE_ENTRY;

typedef struct _VFS_LOOKUP_TABLE
{
    uint32_t dwNameChecksum;
    PACKFILE_ENTRY *pArchiveEntry;
} VFS_LOOKUP_TABLE;

static uint32_t * const g_pcArchives = (uint32_t*)0x01BDB214;
static PACKFILE * const g_pArchives = (PACKFILE*)0x01BA7AC8;
#define VFS_LOOKUP_TABLE_SIZE 20713
static VFS_LOOKUP_TABLE * const g_pVfsLookupTable = (VFS_LOOKUP_TABLE*)0x01BB2AC8;
static BOOL * const g_pbVfsIgnoreTblFiles = (BOOL*)0x01BDB21C;

typedef BOOL (*PFN_LOAD_PACKFILE)(const char *pszFileName, const char *pszDir);
static const PFN_LOAD_PACKFILE VfsLoadPackfile = (PFN_LOAD_PACKFILE)0x0052C070;

typedef PACKFILE *(*PFN_VFS_FIND_PACKFILE)(const char *pszFilename);
static const PFN_VFS_FIND_PACKFILE VfsFindPackfile = (PFN_VFS_FIND_PACKFILE)0x0052C1D0;

typedef uint32_t (*PFN_VFS_CALC_FILENAME_CHECKSUM)(const char *pszFileName);
static const PFN_VFS_CALC_FILENAME_CHECKSUM VfsCalcFileNameChecksum = (PFN_VFS_CALC_FILENAME_CHECKSUM)0x0052BE70;

typedef uint32_t (*PFN_VFS_ADD_FILE_TO_LOOKUP_TABLE)(const PACKFILE_ENTRY *pPackfileEntry);
static const PFN_VFS_ADD_FILE_TO_LOOKUP_TABLE VfsAddFileToLookupTable = (PFN_VFS_ADD_FILE_TO_LOOKUP_TABLE)0x0052BCA0;

typedef uint32_t (*PFN_VFS_PROCESS_PACKFILE_HEADER)(PACKFILE *pPackfile, const void *pHeader);
static const PFN_VFS_PROCESS_PACKFILE_HEADER VfsProcessPackfileHeader = (PFN_VFS_PROCESS_PACKFILE_HEADER)0x0052BD10;

typedef uint32_t (*PFN_VFS_ADD_PACKFILE_ENTRIES)(PACKFILE *pPackfile, const void *pBuf, unsigned cFilesInBlock, unsigned *pcAddedEntries);
static const PFN_VFS_ADD_PACKFILE_ENTRIES VfsAddPackfileEntries = (PFN_VFS_ADD_PACKFILE_ENTRIES)0x0052BD40;

typedef uint32_t (*PFN_VFS_SETUP_FILE_OFFSETS)(PACKFILE *pPackfile, unsigned DataOffsetInBlocks);
static const PFN_VFS_SETUP_FILE_OFFSETS VfsSetupFileOffsets = (PFN_VFS_SETUP_FILE_OFFSETS)0x0052BEB0;

/* Network */

typedef struct _NET_ADDR
{
    uint32_t IpAddr;
    uint32_t Port;
} NET_ADDR;

static SOCKET * const g_pNwSocket = (SOCKET*)0x005A660C;

typedef void (*PFN_PROCESS_GAME_PACKETS)(const char *pData, int cbData, const void *pAddr, void *pPlayer);
static const PFN_PROCESS_GAME_PACKETS RfProcessGamePackets = (PFN_PROCESS_GAME_PACKETS)0x004790D0;

typedef void (*PFN_NET_SEND_NOT_RELIABLE_PACKET)(const void *pAddr, const void *pPacket, unsigned cbPacket);
static const PFN_NET_SEND_NOT_RELIABLE_PACKET RfNetSendNotReliablePacket = (PFN_NET_SEND_NOT_RELIABLE_PACKET)0x0052A080;

typedef void (*PFN_NW_ADDR_TO_STR)(char *pszDest, int cbDest, NET_ADDR *pAddr);
static const PFN_NW_ADDR_TO_STR NwAddrToStr = (PFN_NW_ADDR_TO_STR)0x00529FE0;

/* HUD */

#define HUD_POINTS_COUNT 48
static POINT * const g_pHudPosData640 = (POINT*)0x00637868;
static POINT * const g_pHudPosData800 = (POINT*)0x006373D0;
static POINT * const g_pHudPosData1024 = (POINT*)0x00637230;
static POINT * const g_pHudPosData1280 = (POINT*)0x00637560;
static POINT * const g_pHudPosData = (POINT*)0x006376E8;

/* Chat */

typedef void (*PFN_CHAT_PRINT)(CString strText, unsigned ColorId, CString Prefix);
static PFN_CHAT_PRINT const ChatPrint = (PFN_CHAT_PRINT)0x004785A0;

/* Players */

typedef struct _PLAYER_STATS
{
    uint16_t field_0;
    int16_t iScore;
    int16_t cCaps;
} PLAYER_STATS;

typedef struct _CPlayerNetData
{
    NET_ADDR Addr;
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
} CPlayerNetData;

#pragma pack(push, 1)

typedef struct _SKeyState
{
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	__int16 ScanCodes[2];
	__int16 MouseBtnId;
	__int16 field_1A;
} SKeyState;

typedef struct _CGameControls
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
} CGameControls;

struct _CPlayer;
struct _CEntity;

typedef struct _CAMERA
{
    struct _CEntity *pEntity;
    struct _CPlayer *pPlayer;
    enum { RF_CAM_1ST_PERSON, RF_CAM_3RD_PERSON, RF_CAM_FREE} Type;
} CAMERA;

typedef struct _CPlayer
{
	struct _CPlayer *pNext;
	struct _CPlayer *pPrev;
	CString strName;
	int FireFlags;
	int hEntity;
	int EntityClsId;
	CVector3 field_1C;
	int field_28;
	PLAYER_STATS *pStats;
	char bBlueTeam;
	char bCollide;
	char field_32;
	char field_33;
	void *pWeaponMesh;
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
	CAMERA *pCamera;
	int xViewport;
	int yViewport;
	int cxViewport;
	int cyViewport;
	float fFov;
	int ViewMode;
	int field_E0;
	CGameControls Controls;
	int field_F2C;
	int field_F30;
	BYTE field_F34[4];
	char field_F38;
	char field_F39;
	char field_F3A;
	char bHudVisible;
	int field_F3C;
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
	int field_F58;
	int MpCharacter;
	int field_F60;
	int field_F64;
	int field_F68;
	int field_F6C;
	int field_F70;
	int field_F74;
	int field_F78;
	int field_F7C;
	int field_F80;
	int field_F84;
	int field_F88;
	int field_F8C;
	int field_F90;
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
	int field_FC4;
	int field_FC8;
	int field_FCC;
	int field_FD0;
	int field_FD4;
	int field_FD8;
	int field_FDC;
	int field_FE0;
	int field_FE4;
	int field_FE8;
	int field_FEC;
	int field_FF0;
	CVector3 field_FF4;
	CVector3 field_1000;
	CVector3 field_100C;
	int field_1018;
	int field_101C;
	int field_1020;
	int field_1024;
	int field_1028;
	int field_102C;
	int field_1030;
	int field_1034;
	int field_1038;
	int field_103C;
	int field_1040;
	int field_1044;
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
	int field_1088;
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
	int field_11DC;
	int field_11E0;
	int field_11E4[7];
	CPlayerNetData *pNwData;
} CPlayer;

static CPlayer ** const g_ppPlayersList = (CPlayer**)0x007C75CC;
static CPlayer ** const g_ppLocalPlayer = (CPlayer**)0x007C75D4;

constexpr auto KillLocalPlayer = (void(*)())0x004757A0;

/* Object */

enum EObjectType
{
    OT_ENTITY = 0,
    OT_ITEM = 1,
    OT_WEAPON = 2,
    OT_3 = 3,
    OT_CLUTTER = 4,
    OT_TRIGGER = 5,
    OT_EVENT = 6,
    OT_7 = 7,
    OT_8 = 8,
    OT_MOVER = 9,
    OT_10 = 10,
};

typedef struct SPosRotUnk
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
} SPosRotUnk;

struct CAnimMesh;
typedef struct CAnimMesh CAnimMesh;

typedef struct _CObject
{
    int field_0;
    CVector3 field_4;
    int field_10;
    int field_14;
    CString strName;
    int Uid;
    enum EObjectType Type;
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
    int field_1AC;
    float field_1B0;
    int field_1B4;
    int field_1B8;
    int field_1BC;
    CVector3 field_1C0;
    float field_1CC;
    int field_1D0[6];
    int field_1E8;
    int field_1EC;
    int field_1F0;
    int field_1F4;
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
    struct _CObject *pNextObj;
} CObject;

/* Entity */

typedef void ENTITY_CLASS;
typedef void SMovementMode;

typedef struct SWeaponSelection
{
    int field_0;
    int WeaponClsId;
    int WeaponClsId2;
} SWeaponSelection;

typedef struct SEntityMotion
{
    CVector3 vRotChange;
    CVector3 vPosChange;
    int field_18;
    float field_1C;
    float field_20;
} SEntityMotion;

typedef struct _CEntity
{
    CObject Head;
    int field_290;
    ENTITY_CLASS *pClass;
    int ClassId;
    ENTITY_CLASS *pClass2;
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
    int field_75C;
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
    int field_80C;
    int field_810;
    int field_814[4];
    __int16 KillType;
    __int16 field_826;
    int field_828[9];
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
    int field_8D0[206];
    int field_C08[200];
    int field_F28[200];
    int field_1248[6];
    int field_1260;
    int field_1264;
    int field_1268[71];
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
    int field_13F0[4];
    int field_1400;
    char field_1404[16];
    int field_1414;
    char field_1418[16];
    int field_1428;
    int field_142C;
    CPlayer *pLocalPlayer;
    CVector3 vPitchMin;
    CVector3 vPitchMax;
    int field_144C[8];
    int hUnkEntity;
    int field_1470;
    int AmbientColor;
    int field_1478;
    int field_147C;
    int field_1480;
    int field_1484;
    int field_1488;
    int RespawnVfx;
    int field_1490;
} CEntity;

typedef CEntity *(*PFN_HANDLE_TO_ENTITY)(uint32_t hEntity);
static PFN_HANDLE_TO_ENTITY const HandleToEntity = (PFN_HANDLE_TO_ENTITY)0x00426FC0;

static unsigned * const g_pRiotStickClassId = (unsigned*)0x00872468;

/* Window messages */

typedef void (*PFN_MESSAGE_HANDLER)(UINT, WPARAM, LPARAM);
typedef unsigned (*PFN_ADD_MSG_HANDLER)(PFN_MESSAGE_HANDLER);
static const PFN_ADD_MSG_HANDLER RfAddMsgHandler = (PFN_ADD_MSG_HANDLER)0x00524AE0;

static PFN_MESSAGE_HANDLER * const g_MsgHandlers = (PFN_MESSAGE_HANDLER*)0x01B0D5A0;
static uint32_t * const g_pcMsgHandlers = (uint32_t*)0x01B0D760;

/* Weapons */

typedef struct _WEAPON_CLASS
{
    CString strName;
    CString strDisplayName;
    BYTE Rest[0x550 - 2 * sizeof(CString)];
} WEAPON_CLASS;

static WEAPON_CLASS * const g_pWeaponClasses = (WEAPON_CLASS*)0x0085CD08;
static char ** const g_ppszStringsTable = (char**)0x007CBBF0;

/* Window */

static HWND * const g_phWnd = (HWND*)0x01B0D748;
static uint8_t * const g_pbIsActive = (uint8_t*)0x01B0D750;
static uint8_t * const g_pbClose = (uint8_t*)0x01B0D758;
static uint8_t * const g_pbMouseInitialized = (uint8_t*)0x01885461;
static uint32_t * const g_pcRedrawServer = (uint32_t*)0x01775698;

/* Server */

static NET_ADDR * const g_pServAddr = (NET_ADDR*)0x0064EC5C;
static CString * const g_pstrServName = (CString*)0x0064EC28;

/* Other */

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

enum EGameLang
{
    LANG_EN = 0,
    LANG_GR = 1,
    LANG_FR = 2,
};

static char * const g_pszRootPath = (char*)0x018060E8;
static float * const g_fFps = (float*)0x005A4018;
static uint32_t * const g_pSimultaneousPing = (uint32_t*)0x00599CD8;
static CString * const g_pstrLevelName = (CString*)0x00645FDC;
static int * const g_pBigFontId = (int*)0x006C74C0;
static float * const g_pfMinFramerate = (float*)0x005A4024;
static uint8_t * const g_pbNetworkGame = (uint8_t*)0x0064ECB9;
static uint8_t * const g_pbLocalNetworkGame = (uint8_t*)0x0064ECBA;
static uint32_t * const g_pbDedicatedServer = (uint32_t*)0x01B0D75C;
static bool * const g_pDbgWeapon = (bool*)0x007CAB59;

typedef const char *(*PFN_GET_JOIN_FAILED_STR)(unsigned Reason);
static PFN_GET_JOIN_FAILED_STR const RfGetJoinFailedStr = (PFN_GET_JOIN_FAILED_STR)0x0047BE60;

typedef void (*PFN_BEEP)(unsigned u1, unsigned u2, unsigned u3, float fVolume);
static PFN_BEEP const RfBeep = (PFN_BEEP)0x00505560;

typedef void (*PFN_DRAW_CONSOLE_AND_PROCESS_KBD_FIFO)(BOOL bServer);
static PFN_DRAW_CONSOLE_AND_PROCESS_KBD_FIFO const RfDrawConsoleAndProcessKbdFifo = (PFN_DRAW_CONSOLE_AND_PROCESS_KBD_FIFO)0x0050A720;

typedef void (*PFN_INIT_GAME)(void);
static PFN_INIT_GAME const RfInitGame = (PFN_INIT_GAME)0x004B13F0;

typedef void (*PFN_CLEANUP_GAME)(void);
static PFN_CLEANUP_GAME const RfCleanupGame = (PFN_CLEANUP_GAME)0x004B2D40;

typedef char *(*PFN_GET_FILE_EXT)(char *pszPath);
static PFN_GET_FILE_EXT const GetFileExt = (PFN_GET_FILE_EXT)0x005143F0;

typedef unsigned (*PFN_GET_WIDTH)(void);
static const PFN_GET_WIDTH RfGetWidth = (PFN_GET_WIDTH)0x0050C640;

typedef unsigned (*PFN_GET_HEIGHT)(void);
static const PFN_GET_HEIGHT RfGetHeight = (PFN_GET_HEIGHT)0x0050C650;

typedef unsigned (*PFN_GET_GAME_TYPE)(void);
static const PFN_GET_GAME_TYPE RfGetGameType = (PFN_GET_GAME_TYPE)0x00470770;

typedef unsigned (*PFN_GET_PLAYERS_COUNT)(void);
static const PFN_GET_PLAYERS_COUNT RfGetPlayersCount = (PFN_GET_PLAYERS_COUNT)0x00484830;

typedef uint8_t (*PFN_GET_TEAM_SCORE)(void);
static const PFN_GET_TEAM_SCORE CtfGetRedScore = (PFN_GET_TEAM_SCORE)0x00475020;
static const PFN_GET_TEAM_SCORE CtfGetBlueScore = (PFN_GET_TEAM_SCORE)0x00475030;
static const PFN_GET_TEAM_SCORE TdmGetRedScore = (PFN_GET_TEAM_SCORE)0x004828F0;
static const PFN_GET_TEAM_SCORE TdmGetBlueScore = (PFN_GET_TEAM_SCORE)0x00482900;

typedef void (*PFN_SPLIT_SCREEN)(void);
static const PFN_SPLIT_SCREEN RfSplitScreen = (PFN_SPLIT_SCREEN)0x00480D30;

typedef void (*PFN_KICK_PLAYER)(CPlayer *pPlayer);
static const PFN_KICK_PLAYER RfKickPlayer = (PFN_KICK_PLAYER)0x0047BF00;

typedef void (*PFN_BAN_IP)(NET_ADDR *pAddr);
static const PFN_BAN_IP RfBanIp = (PFN_BAN_IP)0x0046D0F0;

typedef void (*PFN_PACKET_HANDLER)(BYTE *pData, NET_ADDR *pAddr);
static const PFN_PACKET_HANDLER RfHandleNewPlayerPacket = (PFN_PACKET_HANDLER)0x0047A580;

typedef void(*PFN_RENDER_PLAYER_ARM)(CPlayer *pPlayer);
static const PFN_RENDER_PLAYER_ARM RenderPlayerArm = (PFN_RENDER_PLAYER_ARM)0x004A2B30;
static const PFN_RENDER_PLAYER_ARM RenderPlayerArm2 = (PFN_RENDER_PLAYER_ARM)0x004AB1A0;

typedef void(*PFN_SETUP_PLAYER_WEAPON_MESH)(CPlayer *pPlayer, int WeaponClsId);
static const PFN_SETUP_PLAYER_WEAPON_MESH SetupPlayerWeaponMesh = (PFN_SETUP_PLAYER_WEAPON_MESH)0x004AA230;

constexpr auto HandleCtrlInGame = (void(*)(CPlayer *pPlayer, EGameCtrl KeyId, char WasPressed))0x004A6210;

typedef bool(*PFN_IS_PLAYER_ENTITY_INVALID)(CPlayer *pPlayer);
constexpr auto IsPlayerEntityInvalid = (PFN_IS_PLAYER_ENTITY_INVALID)0x004A4920;

constexpr auto RegReadDword = (int(*)(char *pszSubKeyName, LPCSTR lpValueName, DWORD *lpData, int DefaultValue))0x004A4920;

constexpr bool *g_pbDirectInputDisabled = (bool*)0x005A4F88;


/* Graphics */

typedef void (*PFN_OBJECT_RENDER)(CObject *pObj);
static const PFN_OBJECT_RENDER RfRenderEntity = (PFN_OBJECT_RENDER)0x00421850;

typedef void (*PFN_GR_FLUSH_BUFFERS)(void);
static const PFN_GR_FLUSH_BUFFERS GrFlushBuffers = (PFN_GR_FLUSH_BUFFERS)0x00559D90;

constexpr auto GrLoadFont = (int(*)(const char *pszFileName, int a2))0x0051F6E0;
constexpr auto GrSetViewMatrix = (void(*)(CMatrix3 *pMatRot, CVector3 *pPos, float fFov, int a4, int a5))0x00517EB0;
constexpr auto GrSetViewMatrixD3D = (void(*)(CMatrix3 *pMatRot, CVector3 *pPos, float fFov, int a4, int a5))0x00547150;

static int *g_pRfWndWidth = (int*)0x017C7BC4;
static int *g_pRfWndHeight = (int*)0x017C7BC8;

static D3DCAPS8 *g_pRfGrDeviceCaps = (D3DCAPS8*)0x01CFCAC8;
constexpr uint32_t *g_pDrawTextUnk = (uint32_t*)0x17C7C5C;

/* RF stdlib functions are not compatible with GCC */

typedef FILE *(*PFN_FOPEN)(const char *pszPath, const char *pszMode);
static const PFN_FOPEN RfFopen = (PFN_FOPEN)0x00574FFE;

typedef void (*PFN_FSEEK)(FILE *pFile, uint32_t Offset, uint32_t Direction);
static const PFN_FSEEK RfSeek = (PFN_FSEEK)0x00574F14;

typedef void (*PFN_DELETE)(void *pMem);
static const PFN_DELETE RfDelete = (PFN_DELETE)0x0057360E;

typedef void *(*PFN_MALLOC)(uint32_t cbSize);
static const PFN_MALLOC RfMalloc = (PFN_MALLOC)0x00573B37;

//#pragma pack(pop)

#endif // RF_H_INCLUDED
