#ifndef RF_H_INCLUDED
#define RF_H_INCLUDED

#include <stdint.h>
#include <stdio.h>
#include <windows.h>

typedef BOOL WINBOOL;

#include <d3d8.h>

//#pragma pack(push, 1)

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

struct _CPlayer;
struct _CEntity;

typedef struct _CAMERA
{
    struct _CEntity *pEntity;
    struct _CPlayer *pPlayer;
    enum{RF_CAM_1ST_PERSON, RF_CAM_3RD_PERSON, RF_CAM_FREE} Type;
} CAMERA;

typedef struct _CPlayer
{
    struct _CPlayer *pNext;
    struct _CPlayer *pPrev;
    CString strName;
    uint32_t field_10;
    uint32_t hEntity;
    uint32_t EntityClassId;
    uint32_t Unknown[4];
    PLAYER_STATS *pStats;
    uint8_t bBlueTeam;
    uint32_t Unknown2[36];
    CAMERA *pCamera;
    uint32_t Unknown3[1102];
    CPlayerNetData *pNetData;
} CPlayer;

static CPlayer ** const g_ppPlayersList = (CPlayer**)0x007C75CC;
static CPlayer ** const g_ppLocalPlayer = (CPlayer**)0x007C75D4;

/* Object */

typedef struct _CObject
{
    uint8_t Unknown[0x2C];
    uint32_t Handle;
} CObject;

/* Entity */

typedef struct _CEntity
{
    CObject Head;
    uint32_t Unknown[32+1+124];
    int32_t WeaponClassId;
    /* ... */
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

static char * const g_pszRootPath = (char*)0x018060E8;
static float * const g_fFps = (float*)0x005A4018;
static uint32_t * const g_pSimultaneousPing = (uint32_t*)0x00599CD8;
static CString * const g_pstrLevelName = (CString*)0x00645FDC;
static int * const g_pBigFontId = (int*)0x006C74C0;
static float * const g_pfMinFramerate = (float*)0x005A4024;
static uint8_t * const g_pbNetworkGame = (uint8_t*)0x0064ECB9;
static uint8_t * const g_pbLocalNetworkGame = (uint8_t*)0x0064ECBA;
static uint32_t * const g_pbDedicatedServer = (uint32_t*)0x01B0D75C;

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

/* Graphics */

typedef void (*PFN_OBJECT_RENDER)(CObject *pObj);
static const PFN_OBJECT_RENDER RfRenderEntity = (PFN_OBJECT_RENDER)0x00421850;

typedef void (*PFN_GR_FLUSH_BUFFERS)(void);
static const PFN_GR_FLUSH_BUFFERS GrFlushBuffers = (PFN_GR_FLUSH_BUFFERS)0x00559D90;

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
