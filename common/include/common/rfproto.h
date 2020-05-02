/*****************************************************************************
*
*  PROJECT:     Open Faction
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        include/rfproto.h
*  PURPOSE:     RF protocol documentation
*  DEVELOPERS:  Rafal Harabien
*
*****************************************************************************/

#ifndef _RFPROTO_H
#define _RFPROTO_H

#include <stdint.h>

static_assert(sizeof(float) == 4);

#ifdef __BIG_ENDIAN__
#error Big Endian not supported
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4200)
#endif

#pragma pack(push, 1)

#define RF_MAX_NAME_LEN 20
#define RF_MAX_LEVEL_NAME_LEN (MAX_PATH-1)
#define RF_MAX_PASSWORD_LEN 22
#define RF_TRACKER_HOST "rf.thqmultiplay.net"
#define RF_TRACKER_PORT 18444

enum rfMainPacketType
{
    RF_GAME     = 0x00,
    RF_RELIABLE = 0x01,
    RF_TRACKER  = 0x02,
};

/**
 * @page packets Game packets
 */

/**
 * Packet type
 * Type of normal packet. See rfPacketHeader.
 */
enum rfPacketType
{
    RF_SERVER_INFO_REQUEST    = 0x00,
    RF_SERVER_INFO            = 0x01,
    RF_JOIN_REQUEST           = 0x02,
    RF_JOIN_SUCCESS           = 0x03,
    RF_JOIN_FAILED            = 0x04,
    RF_NEW_PLAYER             = 0x05,
    RF_PLAYERS_LIST           = 0x06,
    RF_PLAYER_LEFT            = 0x07,
    RF_SERVER_STOP            = 0x08,
    RF_STATE_REQUEST          = 0x09,
    RF_STATE_END              = 0x0A,
    RF_CLIENT_IN_GAME         = 0x0B, /* sent after joining and when player exit menu screen */
    RF_MESSAGE                = 0x0C,
    RF_NAME_CHANGE            = 0x0D,
    RF_SPAWN_REQUEST          = 0x0E,
    RF_TRIGGER_ACTIVATE       = 0x0F,
    RF_USE_REQUEST            = 0x10,
    RF_GEOMOD_STATE           = 0x11, /* unknown */
    RF_GLASS_STATE            = 0x12,
    /*  13  remote_charges_state */
    RF_SUICIDE                = 0x14,
    RF_LEVEL_END              = 0x15, // enter limbo
    RF_LEVEL_CHANGE           = 0x16, // leave limbo
    RF_TEAM_CHANGE            = 0x17,
    RF_PING                   = 0x18,
    RF_PONG                   = 0x19,
    RF_STATS                  = 0x1A,
    RF_PLAYER_RATE_CHANGE     = 0x1B,
    /*  1c  select_weapon_request */
    RF_CLUTTER_STATE          = 0x1D,
    RF_CLUTTER_KILL           = 0x1E,
    RF_FLAG_STEAL             = 0x1F,
    RF_FLAG_CAPTURE           = 0x20,
    RF_FLAGS_STATE            = 0x21,
    RF_FLAG_RETURN            = 0x22,
    RF_FLAG_DROP              = 0x23,
    RF_DETONATE_REMOTE_CHARGE = 0x24,
    RF_ITEM_UPDATE            = 0x25,
    RF_ENTITY_UPDATE          = 0x26,
    RF_ENTITY_KILL            = 0x27,
    RF_ITEM_APPLY             = 0x28,
    RF_GEOMOD                 = 0x29,
    /*  2a  mover_update
        2b  respawn */
    RF_ENTITY_SPAWN           = 0x2C,
    RF_ITEM_SPAWN             = 0x2D,
    RF_RELOAD                 = 0x2E,
    RF_RELOAD_REQUEST         = 0x2F,
    RF_SHOOT                  = 0x30,
    RF_FALL_DAMAGE            = 0x31,
    RF_RCON_REQUEST           = 0x32,
    RF_RCON_COMMAND           = 0x33,
    RF_SOUND                  = 0x34,
    RF_TEAM_SCORES            = 0x35,
    RF_GLASS_KILL             = 0x36,
};

/**
 * Packet header
 * Header of each normal packet.
 */
typedef struct _rfPacketHeader
{
    uint8_t type;
    uint16_t size;
} rfPacketHeader;

enum rfReliablePacketType
{
    RF_REL_REPLY   = 0x00,
    RF_REL_PACKETS = 0x01,
    RF_REL_JOIN_03 = 0x03,
    RF_REL_JOIN_06 = 0x06,
    RF_REL_JOIN_05 = 0x05,
};

/**
 * Reliable packet
 * Used for delivering ordered reliable packets. It has first byte set to RF_RELIABLE.
 */
typedef struct _rfReliable
{
    uint8_t type; /* usually RF_REL_PACKETS */
    uint8_t unknown;
    uint16_t id; /* Packet ID */
    uint16_t len; /* Length of data */
    uint32_t ticks; /* Milliseconds since client/server start */
    uint8_t data[];
} rfReliable;

/**
 * Synchronization reply packet
 * Special type of packet. It has first byte set to 01.
 */
typedef struct _rfReply
{
    uint8_t type; /* RF_REL_REPLY */
    uint8_t unknown; /* 0x00 */
    uint16_t unknown2;  /* 0x0000 */
    uint16_t len; /* 0x0004 */
    uint32_t ticks; /* cn from packet which reply is for */
    uint16_t packet_id; /* n from packet which reply is for */
    uint16_t unknown3; /* 0x0000 */
} rfReply;

enum rfProtocolVersion
{
    RF_VER_10_11 = 0x87, /* 1.0 and 1.1 servers */
    RF_VER_12    = 0x89, /* 1.2 servers */
    RF_VER_13    = 0x91, /* 1.3 servers (unofficial path) */
};

enum rfGameType
{
    RF_DM     = 0x00,
    RF_CTF    = 0x01,
    RF_TEAMDM = 0x02,
};

enum rfServerFlags
{
    RF_SIF_DEDICATED = 0x01,
    RF_SIF_NOT_LAN   = 0x02,
    RF_SIF_PASSWORD  = 0x04,
};

typedef struct _rfServerInfo
{
    uint8_t type; /* RF_SERVER_INFO */
    uint16_t size; /* Size of the following part */
    uint8_t version; /* Supported version of protocol */
    char rest[];
    /*char name[]; * Server name *
    uint8_t game_type; * Game type (RF_DM, RF_TEAMDM, RF_CTF) *
    uint8_t players_count; * Players count on the server *
    uint8_t max_players_count; * Maximal players count on the server *
    char level[]; * Map name *
    char mod[]; * MOD activated on the server *
    uint8_t flags; * Bitfield (RF_SIF_*) * */
} rfServerInfo;

typedef struct _rfJoinRequest
{
    uint8_t type; /* RF_JOIN_REQUEST */
    uint16_t size; /* Size of the following part */
    uint8_t version; /* Supported version of protocol */
    char rest[];
    /*char name[];
    uint32_t entity_type; * 0x5 *
    char password[];
    uint32_t connection_speed; * In bytes/second *
    uint32_t meshes_vpp_checksum; * exists only if version==RF_VER_10_11 *
    uint32_t meshes_vpp_size; * exists only if version==RF_VER_10_11 *
    uint32_t motions_vpp_checksum; * exists only if version==RF_VER_10_11 *
    uint32_t motions_vpp_size; * exists only if version==RF_VER_10_11 *
    uint32_t tables_vpp_checksum; * not a public algorithm, for 1.2 it's 7e 40 c2 1a *
    uint32_t tables_vpp_size; * Size of tables.vpp *
    uint32_t mod_vpp_checksum; * Checksum of mod VPP if exists (modname.vpp) *
    uint32_t mod_vpp_size; * Size of mod VPP if exists (modname.vpp) *
    [struct rfJoinEx joinEx;]*/
} rfJoinRequest;

typedef struct _rfJoinSuccess
{
    uint8_t type; /* RF_JOIN_SUCCESS */
    uint16_t size; /* Size of the following part */
    char rest[];
    /*char level[]; * Name of current map RFL file *
    rfJoinSuccess2; */
} rfJoinSuccess;

enum rfGameOptions
{
    RF_GO_DEFAULT       = 0x0402,
    RF_GO_TEAM_DAMAGE   = 0x0240,
    RF_GO_FALL_DAMAGE   = 0x0080,
    RF_GO_WEAPONS_STAY  = 0x0010,
    RF_GO_FORCE_RESPAWN = 0x0020,
    RF_GO_BALANCE_TEAMS = 0x2000,
};

enum rfPlayerFlags
{
    RF_PF_UNKNOWN   = 0x00000004,
    RF_PF_BLUE_TEAM = 0x00000080,
};

typedef struct _rfJoinSuccess2
{
    uint32_t level_checksum; /* RFL file checksum - not a standard alghoritm */
    uint32_t game_type; /* Game type (RF_DM, RF_TEAMDM, RF_CTF) */
    uint32_t game_options; /* Bitfield (RF_GO_*) */
    float level_time; /* Time since level start */
    float time_limit; /* Time limit */
    uint8_t player_id; /* Player ID */
    uint32_t flags; /* Bitfield (RF_PF_*) */
} rfJoinSuccess2;

enum rfJoinFailedReason
{
    RF_JFR_JOIN_ACCEPTED         = 0x00, // how it can be join failed? :D
    RF_JFR_SERVER_IS_FULL        = 0x01,
    RF_JFR_NO_ADDITIONAL_PLAYERS = 0x02,
    RF_JFR_INVALID_PASSWORD      = 0x03,
    RF_JFR_THE_SAME_IP           = 0x04,
    RF_JFR_LEVEL_CHANGING        = 0x05,
    RF_JFR_SERVER_ERROR          = 0x06,
    RF_JFR_DATA_DOESNT_MATCH     = 0x07,
    RF_JFR_UNSUPPOROTED_VERSION  = 0x08,
    RF_JFR_UNKNOWN               = 0x09,
    RF_JFR_BANNED                = 0x0A,
    /* Note: reasons > 0x0A shows the same message as RF_JFR_UNKNOWN (tested in PF) */
};

typedef struct _rfJoinFailed
{
    uint8_t type; /* RF_JOIN_FAILED */
    uint16_t size; /* Size of the following part */
    uint8_t reason; /* Reason of join dening */
} rfJoinFailed;

typedef struct _rfNewPlayer
{
    uint8_t type; /* RF_NEW_PLAYER */
    uint16_t size; /* Size of the following part */
    uint8_t id; /* Player ID */
    uint32_t ip; /* Player IP address */
    uint16_t port; /* Player port */
    uint32_t flags; /* Bitfield (RF_PF_*) */
    uint32_t connection_speed; /* Player connection speed in bytes per second. See rfJoin */
    char name[]; /* Player name */
} rfNewPlayer;

typedef struct _rfPlayer
{
    uint8_t flags;
    uint8_t id;
    /*uint32_t unknown] * exists only if flag 1 is set (flags&1). Example of content: 50 00 00 3B */
    uint32_t unknown2; /* 08, 01 */
    uint32_t ip; /* Player IP address */
    uint16_t port; /* Player port */
    char rest[];
    /* char name[]; * Player name *
    uint8_t team; * Team (0 - red, 1 - blue) */
} rfPlayer;

#define RF_PLAYER_NAME(pplayer) ((pplayer)->rest+(((pplayer)->flags&1) ? 4 : 0))
#define RF_PLAYER_IP(pplayer) (*((uint32_t*)(((char*)(pplayer))+(((pplayer)->flags&1) ? 10 : 6))))
#define RF_PLAYER_TEAM(pplayer) ((pplayer)->rest[(((pplayer)->flags&1) ? 4 : 0)+strlen(RF_PLAYER_NAME(pplayer))+1])
#define RF_PLAYER_TEAM_FAST(pplayer, name_len) ((pplayer)->rest[(((pplayer)->flags&1) ? 4 : 0)+(name_len)+1])
#define RF_PLAYER_SIZE(pplayer) (sizeof(rfPlayer)+1+(((pplayer)->flags&1) ? 4 : 0)+strlen(RF_PLAYER_NAME(pplayer))+1)
#define RF_PLAYER_SIZE_FAST(pplayer, name_len) (sizeof(rfPlayer)+1+(((pplayer)->flags&1) ? 4 : 0)+(name_len)+1)
#define RF_NEXT_PLAYER(pplayer) ((rfPlayer*)(((PBYTE)(pplayer))+RF_PLAYER_SIZE(pplayer)))
#define RF_NEXT_PLAYER_FAST(pplayer, name_len) ((rfPlayer*)(((PBYTE)(pplayer))+RF_PLAYER_SIZE_FAST(pplayer, name_len)))

typedef struct _rfPlayersList
{
    uint8_t type; /* RF_PLAYERS_LIST */
    uint16_t size; /* Size of the following part */
    char rest[];
    /*rfPlayer players[];
    uint8_t unknown; * 02 */
} rfPlayersList;

enum rfLeftReason
{
    RF_LR_UNKNOWN           = 0x00,
    RF_LR_NORMAL            = 0x01,
    RF_LR_KICKED            = 0x02,
    RF_LR_DATA_INCOMPATIBLE = 0x04, /* Note: received when PF wants to download map (?) */
    RF_LR_BETWEEN_LEVELS    = 0x05,
    RF_LR_EMPTY             = 0x06, /* TODO: check if it's polish translation bug */
    RF_LR_NO_LEVEL_FILE     = 0x07,
    /* Note: other reasons shows the same message as RF_LR_UNKNOWN */
};

typedef struct _rfPlayerLeft /* or staterequest */
{
    uint8_t type; /* RF_PLAYER_LEFT */
    uint16_t size; /* Size of the following part */
    uint8_t player_id; /* Player ID */
    uint8_t reason; /* Reason of leaving */
} rfPlayerLeft;

typedef struct _rfStateRequest
{
    uint8_t type; /* RF_STATE_REQUEST */
    uint16_t size; /* Size of the following part */
    char level[];
} rfHaveLevel;

typedef struct _rfTriggerActivate
{
    uint8_t type; /* RF_TRIGGER_ACTIVATE */
    uint16_t size; /* Size of the following part */
    uint32_t uid; /* Trigger UID */
    uint32_t entity_handle; /* Entity handle */
} rfTriggerActivate;

typedef struct _rfGlassState
{
    uint8_t type; /* RF_GLASS_STATE */
    uint16_t size; /* Size of the following part */
    uint8_t rooms_bitmap[]; /* Bitmap of killable level rooms (only rooms with life >= 0): 1 - not broken, 0 - broken */
} rfGlassState;

#define RF_SERVER 0xFF /* Player ID of server when sending a chat message */
#define RF_MAX_MSG_LEN 0xF9

typedef struct _rfMessage
{
    uint8_t type; /* RF_MESSAGE */
    uint16_t size; /* Size of the following part */
    uint8_t player_id; /* Player ID of sender */
    uint8_t is_team_msg; /* 0 - global message, 1 - team message */
    char message[]; /* Message content */
} rfMessage;

typedef struct _rfNameChange
{
    uint8_t type; /* RF_NAME_CHANGE */
    uint16_t size; /* Size of the following part */
    uint8_t player_id; /* Player ID */
    char name[]; /* New name */
} rfNameChange;

typedef struct _rfSpawnRequest
{
    uint8_t type; /* RF_SPAWN_REQUEST */
    uint16_t size; /* Size of the following part */
    uint32_t character; /* Player character (index in pc_multi.tbl) */
    uint8_t player_id; /* Player ID */
} rfSpawnRequest;

typedef struct _rfLevelChange
{
    uint8_t type; /* RF_LEVEL_CHANGE */
    uint16_t size; /* Size of the following part */
    char rest[];
    /*char level[]; * Level name *
    uint32_t level_checksum; * not a public algorithm */
} rfLevelChange;

typedef struct _rfTeamChange
{
    uint8_t type; /* RF_TEAM_CHANGE */
    uint16_t size; /* Size of the following part */
    uint8_t player_id; /* Player ID */
    uint8_t team; /* Team (0 - red, 1 - blue) */
} rfTeamChange;

typedef struct _rfPlayerStats
{
    uint8_t player_id; /* Player ID */
    uint16_t ping; /* Player ping */
    uint8_t unknown; /* Ie. 0xFF */
    int16_t score; /* Player score (points from kills and captures) */
    uint8_t captures; /* Captured flags */
    uint8_t unknown2; /* Ie. 0x00 */
} rfPlayerStats;

typedef struct _rfStats
{
    uint8_t type; /* RF_STATS */
    uint16_t size; /* Size of the following part */
    uint8_t unknown; /* 0x07 */
    uint8_t cPlayers; /* Players count */
    char rest[];
    /* rfPlayerStats players[];
    float level_time; * Time since level start *
    float time_limit; * Level time limit */
} rfStats;

typedef struct _rfPlayerRateChange
{
    uint8_t type; /* RF_PLAYER_RATE_CHANGE */
    uint16_t size; /* Size of the following part */
    uint8_t player_id; /* Player changing rate */
    uint16_t rate; /* New rate value (bytes/second) */
} rfRateChange;

typedef struct _rfClutterState
{
    uint8_t type; /* RF_CLUTTER_STATE */
    uint16_t size; /* Size of the following part */
    uint32_t clutter_count; /* Killable clutter count (only clutter with life >= 0) */
    uint8_t clutter_bitmap[]; /* Killable clutter bitmap: 1 - not broken, 0 - broken */
} rfClutterState;

typedef struct _rfClutterKill
{
    uint8_t type; /* RF_CLUTTER_KILL */
    uint16_t size; /* Size of the following part */
    uint32_t uid; /* Clutter UID */
    uint32_t unknown; /* reason? (0 - melee/energy, 1 - normal fire, 2 - continous fire, 3 - boom) */
} rfClutterKill;

typedef struct _rfFlagSteal
{
    uint8_t type; /* RF_FLAG_STEAL */
    uint16_t size; /* Size of the following part */
    uint8_t player_id; /* Player id of thief */
    uint8_t flags_red; /* Red team score */
    uint8_t flags_blue; /* Blue team score */
} rfFlagSteal;

typedef struct _rfFlagCapture
{
    uint8_t type; /* RF_FLAG_CAPTURE */
    uint16_t size; /* Size of the following part */
    uint8_t team; /* Team which has captured the flag (0 - red, 1 - blue) */
    uint8_t player_id; /* Player Id of flag thief */
    uint8_t flags_red; /* Red team score after capture */
    uint8_t flags_blue; /* Blue team score after capture */
} rfFlagCapture;

typedef struct _rfFlagsState
{
    uint8_t type; /* RF_FLAGS_STATE */
    uint16_t size; /* Size of the following part */
    uint8_t red_flag_player; /* Player ID or 0xFF */
    uint8_t red_flag_in_base; /* Ie. 01,                 exists if red_flag_player == 0xFF */
    float x_red, y_red, z_red; /* Red flag position,     exists if red_flag_player == 0xFF && !red_flag_in_base */
    float rot_matrix_red[9]; /* Rotation matrix,         exists if red_flag_player == 0xFF && !red_flag_in_base */
    uint8_t blue_flag_player; /* Player ID or 0xFF */
    uint8_t blue_flag_in_base; /* Ie. 01,                exists if blue_flag_player == 0xFF */
    float x_blue, y_blue, z_blue; /* Blue flag position, exists if blue_flag_player == 0xFF && !red_flag_in_base */
    float rot_matrix_blue[9]; /* Rotation matrix,        exists if blue_flag_player == 0xFF && !red_flag_in_base  */
} rfFlagsState;

typedef struct _rfFlagReturn
{
    uint8_t type; /* RF_FLAG_RETURN */
    uint16_t size; /* Size of the following part */
    uint8_t team; /* Team which has stolen the flag (0 - red, 1 - blue) */
    uint8_t player_id; /* Player ID */
    uint8_t flags_red; /* Red team score */
    uint8_t flags_blue; /* Blue team score */
} rfFlagReturn;

typedef struct _rfFlagDrop
{
    uint8_t type; /* RF_FLAG_DROP */
    uint16_t size; /* Size of the following part */
    uint8_t team; /* Team which has stolen the flag (0 - red, 1 - blue) */
    uint8_t flags_red; /* Red team score */
    uint8_t flags_blue; /* Blue team score */
    float x, y, z; /* Position of the flag */
} rfFlagDrop;

typedef struct _rfDetonateRemoteCharge
{
    uint8_t type; /* RF_DETONATE_REMOTE_CHARGE */
    uint16_t size; /* Size of the following part */
    uint32_t entity_handle; /* Handle of remote charge owner */
    uint8_t player_id; /* Player ID of remote charge owner */
} rfDetonateRemoteCharge;

/* Note: items created after death are hidden by rfItemApply packet */
typedef struct _rfItemUpdate
{
    uint8_t type; /* RF_ITEM_UPDATE */
    uint16_t size; /* Size of the following part */
    char level_items[25]; /* Bitmap of visible static items (loaded from RFL). Maximal number of visible items is 25*8=200 */
} rfItemUpdate;

enum rfWeapon
{
    RF_REMOTE_CHARGE             = 0x00,
    RF_REMOTE_CHARGE_DETONATOR   = 0x01,
    RF_CONTROL_BATON             = 0x02,
    RF_PISTOL                    = 0x03,
    RF_SILENCED_PISTOL           = 0x04, /* not supported in RF multiplayer */
    RF_SHOTGUN                   = 0x05,
    RF_SNIPER_RIFLE              = 0x06,
    RF_ROCKET_LAUNCHER           = 0x07,
    RF_ASSOULT_RIFLE             = 0x08,
    RF_SUBMACHINE_GUN            = 0x09,
    RF_SUBMACHINE_GUN_SPECIAL    = 0x0A, /* not supported in RF multiplayer */
    RF_GRENADE                   = 0x0B,
    RF_FLAMETHROWER              = 0x0C,
    RF_RIOT_SHIELD               = 0x0D,
    RF_RAIL_GUN                  = 0x0E,
    RF_HEAVY_MACHINE_GUN         = 0x0F,
    RF_PRECISION_RIFLE           = 0x10,
    RF_FUSION_ROCKET_LAUNCHER    = 0x11,

    RF_VAUSS                     = 0x12,
    RF_TANKBOT_CHAINGUN          = 0x13,
    RF_TRIBEAM_LASER             = 0x14,
    RF_LASER                     = 0x15,
    RF_CAPEK_CANE                = 0x16,
    RF_REEPER_CLAW               = 0x17,
    RF_BABY_REEPER_CLAW          = 0x18,
    RF_ROCK_SNAKE_SMASH          = 0x19,
    RF_ROCK_SNAKE_SPIT           = 0x1A,
    RF_BIG_ROCK_SNAKE_SMASH      = 0x1B,
    RF_BIG_ROCK_SNAKE_SPIT       = 0x1C,
    RF_SEA_CREATURE_SONAR_ATTACK = 0x1D,
    RF_DRONE_SMASH               = 0x1E,
    RF_TANKBOT_SMASH             = 0x1F,
    RF_MUTANT_ATTACK_1           = 0x20,
    RF_MUTANT_ATTACK_2           = 0x21,
    RF_HEAP                      = 0x22,
    RF_TORPEDO                   = 0x23,
    RF_APC_MINIGUN               = 0x24,
    RF_JEEP_GUN                  = 0x25,
    RF_FIGHTER_MINIGUN           = 0x26,
    RF_DRILL                     = 0x27,
    RF_DRONE_MISSILE             = 0x28,
    RF_TANKBOT_MISSILE           = 0x29,
    RF_FIGHTER_ROCKET            = 0x2A,
    RF_APC_ROCKET                = 0x2B,

    RF_UNKNOWN_WEAPON            = 0xFF,
};

enum rfEntityDataFlags
{
    RF_EDF_POS_ROT_ANIM = 0x01,
    RF_EDF_UNKNOWN4     = 0x02, /* Usually used with RF_ODF_POS_ROT */
    RF_EDF_WEAPON_TYPE  = 0x04,
    RF_EDF_UNKNOWN3     = 0x08, /* Usually used with RF_ODF_FIRE */
    RF_EDF_ALT_FIRE     = 0x10,
    RF_EDF_HEATH_ARMOR  = 0x20,
    RF_EDF_FIRE         = 0x40,
    RF_EDF_ARMOR_STATE  = 0x80,
};

/* Animation first byte - needs more work */
#define RF_OAF_NO_WEAPON   0x01
#define RF_OAF_CROUCH      0x04
#define RF_OAF_RELOAD      0x10
#define RF_OAF_FAST_RELOAD 0x20

enum rfEntityArmorStateFlags
{
    RF_EDAS_DAMAGE_AMPLIFIER = 0x01,
    RF_EDAS_INVULNERABILITY  = 0x02,
};

enum rfAnimFlags
{
    RF_AF_HIDDEN_WEAPON = 0x01, // not used in multi
    RF_AF_CROUCH        = 0x04,
	RF_AF_ZOOM          = 0x08,
    RF_AF_WEAPON_FIRE   = 0x10, // used only for remote charges and granades
	RF_AF_WEAPON_FIRE2  = 0x20, // unknown
};

typedef struct _rfEntityData
{
    uint32_t entity_handle; /* Entity object handle */
    uint8_t flags; /* Bitfield (RF_ODF_*) */
    uint16_t ticks; /* Milliseconds since app. start, exists if RF_ODF_POS_ROT is set */

    /* Position and rotation */
    float x; /* X coordinate,                         exists if RF_ODF_POS_ROT is set */
    float y; /* Y coordinate,                         exists if RF_ODF_POS_ROT is set */
    float z; /* Z coordinate,                         exists if RF_ODF_POS_ROT is set */
    int16_t angle_x; /* Rotation in X axis (pitch),   exists if RF_ODF_POS_ROT is set */
    int16_t angle_y; /* Rotation in Y axis (yaw),     exists if RF_ODF_POS_ROT is set */

    /* Animation */
    uint8_t anim_flags; /* See rfAnimFlags,           exists if RF_ODF_POS_ROT is set */
    int8_t anim_dir_x; /* Normalized velocity in X axis, exists if RF_ODF_POS_ROT is set */
    int8_t anim_dir_y; /* Normalized velocity in Y axis, exists if RF_ODF_POS_ROT is set */
    int8_t anim_speed; /* Speed, MSB is sign of anim_dir_z, exists if RF_ODF_POS_ROT is set */
    /* Note: you can calculate anim_dir_z from anim_dir_x, anim_dir_y and anim_speed */

    /* Other */
    uint8_t armor_state; /* Bitfield (RF_EDAS_*),     exists if RF_ODF_ARMOR_STATE is set */
    uint8_t weapon; /* Weapon type,                   exists if RF_ODF_WEAPON_TYPE is set */
    uint8_t health; /* Entity health points,          exists if RF_ODF_HEATH_ARMOR is set */
    uint8_t armor; /* Entity armor,                   exists if RF_ODF_HEATH_ARMOR is set */
    uint8_t unknown2; /* Ie. 00,                      exists if RF_ODF_HEATH_ARMOR is set */
    uint8_t unknown3; /* Ie. 00,                      exists if RF_ODF_UNKNOWN3 is set */
    /*uint8_t _unknown3[unknown3*3]; *                exists if RF_ODF_UNKNOWN3 is set *
    uint16_t unknown4; * Ie. 00 00,                   exists if RF_ODF_UNKNOWN4 is set */
} rfEntityData;

typedef struct _rfEntityUpdate
{
    uint8_t type; /* RF_ENTITY_UPDATE */
    uint16_t size; /* Size of the following part */
    rfEntityData entities[]; /* Entities data */
    /*uint32_t terminator; * 0xFFFFFFFF */
} rfObjectUpdate;

enum rfKillFlags
{
    RF_KF_ITEM_UNKNOWN = 0x01, /* Note: it's usually not used by game */
    RF_KF_ITEM         = 0x02, /* Creates item in place of death */
};

typedef struct _rfEntityKill
{
    uint8_t type; /* RF_ENTITY_KILL */
    uint16_t size; /* Size of the following part */
    uint32_t entity_handle; /* Entity handle */
    float unknown; /* Ie. 0xC47A0000 (life?) */
    uint8_t id_killer; /* Player ID of killer */
    uint8_t id_killed; /* Player ID of killed player, probably 0xFF if entity is not player */
    uint16_t animation; /* For suicide - 0x5, headshot - 0x9 */
    uint8_t flags; /* Bitfield (RF_KF_*) */
    uint16_t unknown2; /* Ie. 0x0000,  (its char unknown2[]; char unknown3;)                    exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    uint32_t item_type; /* Type of item in place of killed player, Note: it must not be weapon, exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    char unknown4[8]; /* Ie. FF FF FF FF 80 00 00 00,                                           exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    uint32_t item_handle; /* Item handle,                                                       exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    float x; /* X coordinate of item,                                                           exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    float y; /* Y coordinate of item,                                                           exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    float z; /* Z coordinate of item,                                                           exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
    float rot_matrix[3][3]; /* Rotation matrix,                                                 exists if RF_KF_ITEM or RF_KF_ITEM_UNKNOWN is set */
} rfEntityKill;

typedef struct _rfItemApply
{
    uint8_t type; /* RF_ITEM_APPLY */
    uint16_t size; /* Size of the following part */
    uint32_t item_handle; /* Item handle */
    uint32_t entity_handle; /* Entity handle */
    uint32_t weapon; /* Weapon type, 0xFFFFFFFF if no weapon is given */
    uint32_t clip_ammo; /* Ammunition in current clip or 0xFFFFFFFF */
    uint32_t ammo; /* Reserve ammunition or 0xFFFFFFFF */
} rfItemApply;

typedef struct _rfEntitySpawn
{
    uint8_t type; /* RF_ENTITY_SPAWN */
    uint16_t size; /* Size of the following part */
    char rest[];
/*  char name[]; * Entity name *
    rfEntitySpawn2; */
} rfEntitySpawn;

/* Note: Only loword of handles (first 2 bytes) are known to work for example in kill packet. Hiword seems to have no effect :? */

typedef struct _rfEntitySpawn2
{
    uint8_t team; /* 0 - red, 1 - blue */
    uint8_t entity_type; /* Usually 0x5 (miner1) */
    uint32_t entity_handle; /* Object ID */
    uint32_t unknown2; /* Ie. FF FF FF FF */
    float x; /* X coordinate */
    float y; /* Y coordinate */
    float z; /* Z coordinate */
    float rot_matrix[3][3];
    uint8_t id; /* Player ID, probably 0xFF if entity is not player */
    uint32_t character; /* Entity character (index in pc_multi.tbl) */
    uint32_t weapon; /* Entity weapon */
    uint32_t unknown3; /* Ie. FF FF FF FF */
} rfEntitySpawn2;

typedef struct _rfItemSpawn
{
    uint8_t type; /* RF_ITEM_SPAWN */
    uint16_t size; /* Size of the following part */
    char rest[];
/*  char script_name[];
    uint8_t unknown; * Ie. 00 *
    uint32_t item_type; * Item type ID (entry number in items.tbl) *
    uint32_t respawn_time; * Item respawn time in ms (0xFFFFFFFF if item is temporary) *
    uint32_t count; * $Count or $Count Multi from items.tbl (not value from RFL) *
    uint32_t item_handle; * Item object ID *
    uint16_t item_bit; * Item index in rfObjectUpdate packet, 0xFFFF if item is temporary (created after death) *
    uint8_t unknown2; * 0x75 - enabled, 0x68 - not enabled *
    float x, y, z;
    float rot_matrix[3][3];*/
} rfItem;

typedef struct _rfReload
{
    uint8_t type; /* RF_RELOAD */
    uint16_t size; /* Size of the following part */
    uint32_t entity_handle; /* Entity handle */
    uint32_t weapon; /* Entity weapon type */
    uint32_t ammo; /* Reserve ammunition */
    uint32_t clip_ammo; /* Ammunition in current clip */
} rfReload;

typedef struct _rfReloadRequest
{
    uint8_t type; /* RF_RELOAD_REQUEST */
    uint16_t size; /* Size of the following part */
    uint32_t weapon; /* Player weapon */
} rfReloadRequest;

enum rfShootFlags
{
    RF_SF_ALT_FIRE   = 0x01,
    RF_SF_UNKNOWN    = 0x02,
    RF_SF_NO_POS_ROT = 0x04,
};

typedef struct _rfShoot
{
    uint8_t type; /* RF_SHOOT */
    uint16_t size; /* Size of the following part */
    uint8_t weapon; /* Weapon type */
    uint8_t flags; /* Bitfield (RF_SF_*) */
    uint8_t player_id; /* Player ID. Note: exists only in packets sent by server */
    float x; /* X coordinate of weapon, exists only if RF_SF_NO_POS_ROT is NOT set */
    float y; /* Y coordinate of weapon, exists only if RF_SF_NO_POS_ROT is NOT set */
    float z; /* Z coordinate of weapon, exists only if RF_SF_NO_POS_ROT is NOT set */
    /* unsigned? */
    int16_t direction_x; /* X coordinate of direction vector, exists only if RF_SF_NO_POS_ROT is NOT set */
    int16_t direction_y; /* Y coordinate of direction vector, exists only if RF_SF_NO_POS_ROT is NOT set */
    int16_t direction_z; /* Z coordinate of direction vector, exists only if RF_SF_NO_POS_ROT is NOT set */
    uint8_t unknown; /* exists only if RF_SF_UNKNOWN is set */
} rfShoot;

typedef struct _rfFallDamage
{
    uint8_t type; /* RF_FALL_DAMAGE */
    uint16_t size; /* Size of the following part */
    float force; /* Force? */
} rfFallDemage;

typedef struct _rfSound
{
    uint8_t type; /* RF_SOUND */
    uint16_t size; /* Size of the following part */
    uint16_t sound_id; /* Sound ID from sounds.tbl */
    float x, y, z; /* FF FF FF FF FF FF FF FF FF FF FF FF */
} rfSound;

typedef struct _rfTeamsScores
{
    uint8_t type; /* RF_TEAM_SCORES */
    uint16_t size; /* Size of the following part */
    uint16_t score_red; /* Score of red team */
    uint16_t score_blue; /* Score of blue team */
} rfTeamsScores;

typedef struct _rfGlassKill
{
    uint8_t type; /* RF_GLASS_KILL */
    uint16_t size; /* Size of the following part */
    int32_t room_id; /* 0x7FFFFFFF - index starting from 1 */
    uint8_t explosion; /* 1 or 0 */
    float x, y, z; /* destruction start pos */
    float unknown[3]; /* 0.0f if explosion == 1 */
} rfGlassKill;


/**
 * Packet union
 * Represents each normal game packet
 */
typedef union _rfPacket
{
    rfPacketHeader header;
    rfServerInfo serverInfo;
    rfJoinRequest joinRequest;
    rfJoinSuccess joinSuccess;
    rfJoinFailed joinFailed;
    rfNewPlayer newPlayer;
    rfPlayersList playersList;
    rfPlayerLeft playerLeft;
    rfHaveLevel haveLevel;
    rfMessage message;
    rfNameChange nameChange;
    rfSpawnRequest spawnRequest;
    rfLevelChange levelChange;
    rfTeamChange teamChange;
    rfStats stats;
    rfFlagSteal flagSteal;
    rfFlagCapture flagCapture;
    rfFlagsState flagsState;
    rfFlagReturn flagReturn;
    rfObjectUpdate objectUpdate;
    rfEntityKill entityKill;
    rfEntitySpawn entitySpawn;
    rfReloadRequest reloadRequest;
    rfShoot shoot;
    rfTeamsScores teamsScores;
} rfPacket;

/**
 * @page Tracker packets
 */

enum rfTrackerPacketType
{
    RF_TRACKER_REPLY               = 0x01,
    RF_TRACKER_SERVER_STOPED       = 0x03,
    RF_TRACKER_SERVER_PING         = 0x04,
    RF_TRACKER_SERVER_LIST_REQUEST = 0x05,
    RF_TRACKER_SERVER_LIST         = 0x06,
    RF_TRACKER_SERVER_LIST_END     = 0x07,
};

typedef struct _rfTrackerHeader
{
    uint8_t unknown; /* 0x06 */
    uint16_t type;
    uint32_t seq;
    uint16_t packet_len; /* Length of whole packet (with packet class) */
} rfTrackerHeader;

typedef struct _rfServerAddress
{
    uint32_t ip;
    uint16_t port;
} rfServerAddress;

typedef struct _rfTrackerServerList
{
    uint8_t unknown; /* 0x06 */
    uint16_t type; /* RF_SERVER_LIST */
    uint32_t seq;
    uint16_t packet_len;
    uint8_t packet_servers_count;
    uint32_t servers_count;
    rfServerAddress sl[];
} rfTrackerServerList;

typedef struct _rfTrackerUnknown
{
    uint8_t unknown; /* 0x06 */
    uint16_t type; /* RF_SERVER_LIST */
    uint32_t seq;
    uint16_t packet_len;
    uint8_t unknown2;
} rfTrackerUnknown;

typedef union _rfTracketPacket
{
    rfTrackerHeader header;
    rfTrackerServerList serverList;
    rfTrackerUnknown serverPing;
    rfTrackerUnknown serverStop;
} rfTracketPacket;

#pragma pack(pop)

#endif /* _RFPROTO_H */
