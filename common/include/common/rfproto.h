/*****************************************************************************
*
*  PROJECT:     Open Faction
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        rfproto.h
*  PURPOSE:     RF protocol documentation
*  DEVELOPERS:  Rafał Harabień
*
*****************************************************************************/

// Note: This file assumes a little-endian binary representation of integers and IEEE 754 compatible representation of
//       floating point numbers.

#ifndef _RFPROTO_H
#define _RFPROTO_H

#include <stdint.h>

#ifdef _MSC_VER
#pragma warning(disable : 4200) // nonstandard extension used : zero-sized array in struct/union
#endif

#pragma pack(push, 1)

#define RF_MAX_NAME_LEN 20
#define RF_MAX_LEVEL_NAME_LEN (MAX_PATH-1)
#define RF_MAX_PASSWORD_LEN 22
#define RF_TRACKER_HOST "rf.thqmultiplay.net"
#define RF_TRACKER_PORT 18444

/**
 * Main Packet type
 * First byte of a datagram.
 */
enum RF_MainPacketType
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
 * Type of normal packet. See RF_GamePacketHeader.
 */
enum RF_GamePacketType
{
    RF_GPT_GAME_INFO_REQUEST      = 0x00,
    RF_GPT_GAME_INFO              = 0x01,
    RF_GPT_JOIN_REQUEST           = 0x02,
    RF_GPT_JOIN_ACCEPT            = 0x03,
    RF_GPT_JOIN_DENY              = 0x04,
    RF_GPT_NEW_PLAYER             = 0x05,
    RF_GPT_PLAYERS                = 0x06,
    RF_GPT_LEFT_GAME              = 0x07,
    RF_GPT_END_GAME               = 0x08,
    RF_GPT_STATE_INFO_REQUEST     = 0x09,
    RF_GPT_STATE_INFO_DONE        = 0x0A,
    RF_GPT_CLIENT_IN_GAME         = 0x0B, // sent after joining and when player exit menu screen
    RF_GPT_CHAT_LINE              = 0x0C,
    RF_GPT_NAME_CHANGE            = 0x0D,
    RF_GPT_RESPAWN_REQUEST        = 0x0E,
    RF_GPT_TRIGGER_ACTIVATE       = 0x0F,
    RF_GPT_USE_KEY_PRESSED        = 0x10,
    RF_GPT_PREGAME_BOOLEAN        = 0x11,
    RF_GPT_PREGAME_GLASS          = 0x12,
    RF_GPT_PREGAME_REMOTE_CHARGE  = 0x13,
    RF_GPT_SUICIDE                = 0x14,
    RF_GPT_ENTER_LIMBO            = 0x15, // level ended
    RF_GPT_LEAVE_LIMBO            = 0x16, // level is changing
    RF_GPT_TEAM_CHANGE            = 0x17,
    RF_GPT_PING                   = 0x18,
    RF_GPT_PONG                   = 0x19,
    RF_GPT_NETGAME_UPDATE         = 0x1A,
    RF_GPT_RATE_CHANGE            = 0x1B,
    RF_GPT_SELECT_WEAPON          = 0x1C,
    RF_GPT_CLUTTER_UPDATE         = 0x1D,
    RF_GPT_CLUTTER_KILL           = 0x1E,
    RF_GPT_CTF_FLAG_PICKED_UP     = 0x1F,
    RF_GPT_CTF_FLAG_CAPTURED      = 0x20,
    RF_GPT_CTF_FLAG_UPDATE        = 0x21,
    RF_GPT_CTF_FLAG_RETURNED      = 0x22,
    RF_GPT_CTF_FLAG_DROPED        = 0x23,
    RF_GPT_REMOTE_CHARGE_KILL     = 0x24,
    RF_GPT_ITEM_UPDATE            = 0x25,
    RF_GPT_OBJECT_UPDATE          = 0x26,
    RF_GPT_OBJECT_KILL            = 0x27,
    RF_GPT_ITEM_APPLY             = 0x28,
    RF_GPT_BOOLEAN                = 0x29, // geomod
    RF_GPT_MOVER_UPDATE           = 0x2A, // unused
    RF_GPT_RESPAWN                = 0x2B,
    RF_GPT_ENTITY_CREATE          = 0x2C,
    RF_GPT_ITEM_CREATE            = 0x2D,
    RF_GPT_RELOAD                 = 0x2E,
    RF_GPT_RELOAD_REQUEST         = 0x2F,
    RF_GPT_WEAPON_FIRE            = 0x30,
    RF_GPT_FALL_DAMAGE            = 0x31,
    RF_GPT_RCON_REQUEST           = 0x32,
    RF_GPT_RCON                   = 0x33,
    RF_GPT_SOUND                  = 0x34,
    RF_GPT_TEAM_SCORES            = 0x35,
    RF_GPT_GLASS_KILL             = 0x36,
};

/**
 * Packet header
 * Header of each normal packet.
 */
struct RF_GamePacketHeader
{
    uint8_t type;  // see RF_GamePacketType
    uint16_t size; // length of data without the header
};

enum RF_ReliablePacketType
{
    RF_RPT_REPLY   = 0x00,
    RF_RPT_PACKETS = 0x01,
    RF_RPT_JOIN_03 = 0x03,
    RF_RPT_JOIN_06 = 0x06,
    RF_RPT_JOIN_05 = 0x05,
};

/**
 * Reliable packet
 * Used for delivering ordered reliable packets. It has first byte set to RF_RELIABLE.
 */
struct RF_ReliablePacket
{
    uint8_t type;    // usually RF_RPT_PACKETS
    uint8_t unknown;
    uint16_t id;     // Packet ID
    uint16_t len;    // Length of data
    uint32_t ticks;  // Milliseconds since client/server start
    uint8_t data[];
};

/**
 * Synchronization reply packet
 * Special type of packet. It has first byte set to 01.
 */
struct RF_ReliableReplyPacket
{
    uint8_t type;       // RF_RPT_REPLY
    uint8_t unknown;    // 0x00
    uint16_t unknown2;  // 0x0000
    uint16_t len;       // 0x0004
    uint32_t ticks;     // cn from packet which reply is for
    uint16_t packet_id; // n from packet which reply is for
    uint16_t unknown3;  // 0x0000
};

enum RF_ProtocolVersion
{
    RF_VER_10_11 = 0x87, // 1.0 and 1.1 servers
    RF_VER_12    = 0x89, // 1.2 servers
    RF_VER_13    = 0x91, // 1.3 servers (unofficial path)
};

enum RF_GameType
{
    RF_GT_DM     = 0x00,
    RF_GT_CTF    = 0x01,
    RF_GT_TEAMDM = 0x02,
};

enum RF_ServerFlags
{
    RF_SF_DEDICATED = 0x01,
    RF_SF_NOT_LAN   = 0x02,
    RF_SF_PASSWORD  = 0x04,
};

struct RF_GameInfoPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_GAME_INFO
    uint8_t version;                   // Supported version of protocol
#ifdef PSEUDOCODE
    char name[];                       // Server name
    uint8_t game_type;                 // Game type (RF_DM, RF_TEAMDM, RF_CTF)
    uint8_t players_count;             // Players count on the server
    uint8_t max_players_count;         // Maximal players count on the server
    char level[];                      // Map name
    char mod[];                        // MOD activated on the server
    uint8_t flags;                     // Bitfield (RF_SF_*) *
#else
    char rest[];
#endif
};

struct RF_JoinRequestPacket
{
    struct RF_GamePacketHeader header;    // RF_GPT_JOIN_REQUEST
    uint8_t version;                      // Supported version of protocol
#ifdef PSEUDOCODE
    char name[];
    uint32_t entity_type;                 // 0x5
    char password[];
    uint32_t rate;                        // connection speed in bytes/second
    if (version==RF_VER_10_11) {
        uint32_t meshes_vpp_checksum;
        uint32_t meshes_vpp_size;
        uint32_t motions_vpp_checksum;
        uint32_t motions_vpp_size;
    }
    uint32_t tables_vpp_checksum;         // not a public algorithm, for 1.2 it's 7e 40 c2 1a
    uint32_t tables_vpp_size;             // Size of tables.vpp
    uint32_t mod_vpp_checksum;            // Checksum of mod VPP if exists (modname.vpp)
    uint32_t mod_vpp_size;                // Size of mod VPP if exists (modname.vpp)
    /*struct RF_JoinEx joinEx;*/
#else
    char rest[];
#endif
};

struct RF_JoinAcceptPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_JOIN_ACCEPT
#ifdef PSEUDOCODE
    char level[];                      // Name of current map RFL file
    struct RF_JoinAcceptRest rest;
#else
    char rest[];
#endif
    /**/
};

enum RF_GameOptions
{
    RF_GO_DEFAULT       = 0x0402,
    RF_GO_TEAM_DAMAGE   = 0x0240,
    RF_GO_FALL_DAMAGE   = 0x0080,
    RF_GO_WEAPONS_STAY  = 0x0010,
    RF_GO_FORCE_RESPAWN = 0x0020,
    RF_GO_BALANCE_TEAMS = 0x2000,
};

enum RF_PlayerFlags
{
    RF_PF_UNKNOWN   = 0x00000004,
    RF_PF_BLUE_TEAM = 0x00000080,
};

struct RF_JoinAcceptRest
{
    uint32_t level_checksum; // RFL file checksum - not a standard alghoritm
    uint32_t game_type;      // Game type (RF_GT_DM, RF_GT_TEAMDM, RF_GT_CTF)
    uint32_t game_options;   // Bitfield (RF_GO_*)
    float level_time;        // Time since level start
    float time_limit;        // Time limit
    uint8_t player_id;       // Player ID
    uint32_t flags;          // Bitfield (RF_PF_*)
};

enum RF_JoinDenyReason
{
    RF_JDR_SERVER_IS_FULL        = 0x01,
    RF_JDR_NO_ADDITIONAL_PLAYERS = 0x02,
    RF_JDR_INVALID_PASSWORD      = 0x03,
    RF_JDR_THE_SAME_IP           = 0x04,
    RF_JDR_LEVEL_CHANGING        = 0x05,
    RF_JDR_SERVER_ERROR          = 0x06,
    RF_JDR_DATA_DOESNT_MATCH     = 0x07,
    RF_JDR_UNSUPPOROTED_VERSION  = 0x08,
    RF_JDR_UNKNOWN               = 0x09,
    RF_JDR_BANNED                = 0x0A,
    // Note: reasons > 0x0A shows the same message as RF_JDR_UNKNOWN (tested in PF)
};

struct RF_JoinDenyPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_JOIN_DENY
    uint8_t reason;                    // Reason of join dening (see RF_JoinDenyReason)
};

struct RF_NewPlayerPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_NEW_PLAYER
    uint8_t id;                        // Player ID
    uint32_t ip;                       // Player IP address
    uint16_t port;                     // Player port
    uint32_t flags;                    // Bitfield (see RF_PlayerFlags)
    uint32_t rate;                     // Player connection speed in bytes per second. See RF_JoinRequestPacket
    char name[];                       // Player name
};

struct RF_Player
{
    uint8_t flags;
    uint8_t id;
#ifdef PSEUDOCODE
    if (flags & 1) {
        uint32_t unknown; // Example of content: 50 00 00 3B
    }
    uint32_t unknown2; // 08, 01
    uint32_t ip;       // Player IP address
    uint16_t port;     // Player port
    char name[];       // Player name
    uint8_t team;      // Team (0 - red, 1 - blue)
#else
    char rest[];
#endif
};

struct RF_PlayersPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_PLAYERS
#ifdef PSEUDOCODE
    struct RF_Player players[];
    uint8_t unknown; // 02
#else
    char rest[];
#endif
};

enum RF_LeftReason
{
    RF_LR_UNKNOWN           = 0x00,
    RF_LR_NORMAL            = 0x01,
    RF_LR_KICKED            = 0x02,
    RF_LR_DATA_INCOMPATIBLE = 0x04, // Note: received when PF wants to download map (?)
    RF_LR_BETWEEN_LEVELS    = 0x05,
    RF_LR_EMPTY             = 0x06, // TODO: check if it's polish translation bug
    RF_LR_NO_LEVEL_FILE     = 0x07,
    // Note: other reasons shows the same message as RF_LR_UNKNOWN
};

struct RF_LeftGamePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_LEFT_GAME
    uint8_t player_id;                 // Player ID
    uint8_t reason;                    // Reason of leaving
};

struct RF_StateInfoRequestPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_STATE_INFO_REQUEST
    char level[];
};

struct RF_TriggerActivatePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_TRIGGER_ACTIVATE
    uint32_t uid;                      // Trigger UID
    uint32_t entity_handle;            // Entity handle
};

struct RF_PregameGlassPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_PREGAME_GLASS
    uint8_t rooms_bitmap[];            // Bitmap of killable level rooms (only rooms with life >= 0): 1 - not broken, 0 - broken
};

#define RF_SERVER 0xFF // Player ID of server when sending a chat message
#define RF_MAX_MSG_LEN 0xF9

struct RF_ChatLinePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_CHAT_LINE
    uint8_t player_id;                 // Player ID of sender
    uint8_t is_team_msg;               // 0 - global message, 1 - team message
    char message[];                    // Message content
};

struct RF_NameChangePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_NAME_CHANGE
    uint8_t player_id;                 // Player ID
    char name[];                       // New name
};

struct RF_RespawnRequestPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_RESPAWN_REQUEST
    uint32_t character;                // Player character (index in pc_multi.tbl)
    uint8_t player_id;                 // Player ID
};

struct RF_LeaveLimboPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_LEAVE_LIMBO
#ifdef PSEUDOCODE
    char level[];                      // Level name
    uint32_t level_checksum;           // not a public algorithm
#else
    char rest[];
#endif
};

struct RF_TeamChangePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_TEAM_CHANGE
    uint8_t player_id;                 // Player ID
    uint8_t team;                      // Team (0 - red, 1 - blue)
};

struct RF_PlayerStats
{
    uint8_t player_id; // Player ID
    uint16_t ping;     // Player ping
    uint8_t unknown;   // Ie. 0xFF
    int16_t score;     // Player score (points from kills and captures)
    uint8_t captures;  // Captured flags
    uint8_t unknown2;  // Ie. 0x00
};

struct RF_NetgameUpdatePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_NETGAME_UPDATE
    uint8_t unknown;                   // 0x07
    uint8_t player_count;              // Player count
#ifdef PSEUDOCODE
    struct RF_PlayerStats players[];
    float level_time;                  // Time since level start
    float time_limit;                  // Level time limit
#else
    char rest[];
#endif
};

struct RF_RateChangePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_RATE_CHANGE
    uint8_t player_id;                 // Player changing rate
    uint16_t rate;                     // New rate value (bytes/second)
};

struct RF_PregameClutterPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_PREGAME_CLUTTER
    uint32_t clutter_count;            // Killable clutter count (only clutter with life >= 0)
    uint8_t clutter_bitmap[];          // Killable clutter bitmap: 1 - not broken, 0 - broken
};

struct RF_ClutterKillPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_CLUTTER_KILL
    uint32_t uid;                      // Clutter UID
    uint32_t unknown;                  // reason? (0 - melee/energy, 1 - normal fire, 2 - continous fire, 3 - boom)
};

struct RF_CtfFlagPickedUpPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_CTF_FLAG_PICKED_UP
    uint8_t player_id;                 // Player id of thief
    uint8_t flags_red;                 // Red team score
    uint8_t flags_blue;                // Blue team score
};

struct RF_CtfFlagCapturedPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_CTF_FLAG_CAPTURED
    uint8_t team;                      // Team which has captured the flag (0 - red, 1 - blue)
    uint8_t player_id;                 // Player Id of flag thief
    uint8_t flags_red;                 // Red team score after capture
    uint8_t flags_blue;                // Blue team score after capture
};

struct RF_CtfFlagUpdatePacket
{
    struct RF_GamePacketHeader header;         // RF_GPT_CTF_FLAG_UPDATE
#ifdef PSEUDOCODE
    uint8_t red_flag_player;                   // Player ID or 0xFF
    if (red_flag_player == 0xFF) {
        uint8_t red_flag_in_base;              // Ie. 01
        if (!red_flag_in_base) {
            struct RF_Vector red_flag_pos;     // Red flag position
            struct RF_Matrix red_flag_orient;  // Rotation matrix
        }
    }
    uint8_t blue_flag_player;                  // Player ID or 0xFF
    if (blue_flag_player == 0xFF) {
        uint8_t blue_flag_in_base;             // Ie. 01
        if (!red_flag_in_base) {
            struct RF_Vector blue_flag_pos;    // Blue flag position
            struct RF_Matrix blue_flag_orient; // Rotation matrix
        }
    }
#else
    char rest[];
#endif
};

struct RF_CtfFlagReturnedPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_CTF_FLAG_RETURNED
    uint8_t team;                      // Team which has stolen the flag (0 - red, 1 - blue)
    uint8_t player_id;                 // Player ID
    uint8_t flags_red;                 // Red team score
    uint8_t flags_blue;                // Blue team score
};

struct RF_Vector
{
    float x;
    float y;
    float z;
};

struct RF_Matrix
{
    float data[3][3];
};

struct RF_CtfFlagDropedPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_CTF_FLAG_DROPED
    uint8_t team;                      // Team which has stolen the flag (0 - red, 1 - blue)
    uint8_t flags_red;                 // Red team score
    uint8_t flags_blue;                // Blue team score
    struct RF_Vector pos;              // Position of the flag
};

struct RF_RemoteChargeKillPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_REMOTE_CHARGE_KILL
    uint32_t entity_handle;            // Handle of remote charge owner
    uint8_t player_id;                 // Player ID of remote charge owner
};

// Note: items created after death are hidden by RF_ItemApplyPacket
struct RF_ItemUpdatePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_ITEM_UPDATE
    uint8_t level_items_bitmap[25];    // Bitmap of visible static items (loaded from RFL). Maximal number of visible items is 25*8=200
};

enum RF_Weapon
{
    RF_REMOTE_CHARGE             = 0x00,
    RF_REMOTE_CHARGE_DETONATOR   = 0x01,
    RF_CONTROL_BATON             = 0x02,
    RF_PISTOL                    = 0x03,
    RF_SILENCED_PISTOL           = 0x04, // not supported in RF multiplayer
    RF_SHOTGUN                   = 0x05,
    RF_SNIPER_RIFLE              = 0x06,
    RF_ROCKET_LAUNCHER           = 0x07,
    RF_ASSOULT_RIFLE             = 0x08,
    RF_SUBMACHINE_GUN            = 0x09,
    RF_SUBMACHINE_GUN_SPECIAL    = 0x0A, // not supported in RF multiplayer
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

enum RF_ObjectUpdateFlags
{
    RF_OUF_POS_ROT_ANIM = 0x01,
    RF_OUF_UNKNOWN4     = 0x02, // Usually used with RF_ODF_POS_ROT
    RF_OUF_WEAPON_TYPE  = 0x04,
    RF_OUF_UNKNOWN3     = 0x08, // Usually used with RF_ODF_FIRE
    RF_OUF_ALT_FIRE     = 0x10,
    RF_OUF_HEATH_ARMOR  = 0x20,
    RF_OUF_FIRE         = 0x40,
    RF_OUF_AMP_FLAGS    = 0x80,
};

// Animation first byte - needs more work
#define RF_OAF_NO_WEAPON   0x01
#define RF_OAF_CROUCH      0x04
#define RF_OAF_RELOAD      0x10
#define RF_OAF_FAST_RELOAD 0x20

enum RF_EntityAmpFlags
{
    RF_EAF_DAMAGE_AMPLIFIER = 0x01,
    RF_EAF_INVULNERABILITY  = 0x02,
};

enum RF_EntityStateFlags
{
    RF_AF_HIDDEN_WEAPON = 0x01, // not used in multi
    RF_AF_CROUCH        = 0x04,
    RF_AF_ZOOM          = 0x08,
    RF_AF_WEAPON_FIRE   = 0x10, // used only for remote charges and granades
    RF_AF_WEAPON_FIRE2  = 0x20, // unknown
};

struct RF_ObjectUpdate
{
    uint32_t handle;          // Object object handle
    uint8_t flags;            // Bitfield (RF_ObjectUpdateFlags)
#ifdef PSEUDOCODE
    if (flags & RF_OUF_POS_ROT) {
        uint16_t ticks;       // Milliseconds since app. start
        struct RF_Vector pos; // position,
        int16_t angle_x;      // Rotation in X axis (pitch)
        int16_t angle_y;      // Rotation in Y axis (yaw)
        uint8_t state_flags;  // See RF_EntityStateFlags
        int8_t move_dir_x;    // Normalized velocity in X axis
        int8_t move_dir_y;    // Normalized velocity in Y axis
        int8_t move_speed;    // Speed, MSB is sign of move_dir_z
        // Note: you can calculate move_dir_z from move_dir_x, move_dir_y and move_speed
    }
    if (flags & RF_OUF_AMP_FLAGS) {
        uint8_t amp_flags;    // Bitfield (RF_EntityAmpFlags)
    }
    if (flags & RF_OUF_WEAPON_TYPE) {
        uint8_t weapon;       // Weapon type
    }
    if (flags & RF_OUF_HEATH_ARMOR) {
        uint8_t health;       // Entity health points
        uint8_t armor;        // Entity armor
        uint8_t unknown2;     // Ie. 00
    }
    if (flags & RF_OUF_UNKNOWN3) {
        uint8_t unknown3;     // Ie. 00
        uint8_t _unknown3[unknown3*3];
    }
    if (flags & RF_OUF_UNKNOWN4) {
        uint16_t unknown4;    // Ie. 00 00
    }

#else
    char rest[];
#endif
};

struct RF_ObjectUpdatePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_OBJECT_UPDATE
#ifdef PSEUDOCODE
    RF_ObjectUpdate objects[];         // Objects data
    uint32_t terminator = 0xFFFFFFFF;
#else
    char rest[];
#endif
};

enum RF_ObjectKillFlags
{
    RF_OKF_ITEM_UNKNOWN = 0x01, // Note: it's usually not used by game
    RF_OKF_ITEM         = 0x02, // Creates item in place of death
};

struct RF_ObjectKillPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_OBJECT_KILL
    uint32_t entity_handle;            // Entity handle
    float unknown;                     // Ie. 0xC47A0000 (life?)
    uint8_t id_killer;                 // Player ID of killer
    uint8_t id_killed;                 // Player ID of killed player, probably 0xFF if entity is not player
    uint16_t animation;                // For suicide - 0x5, headshot - 0x9
    uint8_t flags;                     // Bitfield (RF_ObjectKillFlags)
#ifdef PSEUDOCODE
    if (flags & (RF_OKF_ITEM | RF_OKF_ITEM_UNKNOWN)) {
        uint16_t unknown2;             // Ie. 0x0000,  (its char unknown2[]; char unknown3;)
        uint32_t item_type;            // Type of item in place of killed player, Note: it must not be weapon
        char unknown4[8];              // Ie. FF FF FF FF 80 00 00 00,
        uint32_t item_handle;          // Item handle,
        struct RF_Vector item_pos;     // X coordinate of item,
        struct RF_Matrix rot_matrix;   // Rotation matrix,
    }
#else
    char rest[];
#endif
};

struct RF_ItemApplyPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_ITEM_APPLY
    uint32_t item_handle;              // Item handle
    uint32_t entity_handle;            // Entity handle
    uint32_t weapon;                   // Weapon type, 0xFFFFFFFF if no weapon is given
    uint32_t ammo;                     // Ammunition in current magazine or 0xFFFFFFFF
    uint32_t clip_ammo;                // Ammunition in other magazines or 0xFFFFFFFF
};

struct RF_EntityCreatePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_ENTITY_CREATE
#ifdef PSEUDOCODE
    char name[];                       // Entity name
    struct RF_EntityCreatePacketRest rest;
#else
    char rest[];
#endif
};

// Note: Only loword of handles (first 2 bytes) are known to work for example in kill packet. Hiword seems to have no effect.

struct RF_EntityCreatePacketRest
{
    uint8_t team;            // 0 - red, 1 - blue
    uint8_t entity_type;     // Usually 0x5 (miner1)
    uint32_t entity_handle;  // Object ID
    uint32_t unknown2;       // Ie. 0xFFFFFFFF
    struct RF_Vector pos;    // position vector
    struct RF_Matrix orient; // orientation (rotation matrix)
    uint8_t player_id;       // Player ID, probably 0xFF if entity is not player
    uint32_t character;      // Entity character (index in pc_multi.tbl)
    uint32_t weapon;         // Entity weapon
    uint32_t unknown3;       // Ie. 0xFFFFFFFF
};

struct RF_ItemCreatePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_ITEM_CREATE
#ifdef PSEUDOCODE
    char script_name[];                // Zero-terminated string
    uint8_t unknown;                   // Ie. 00
    uint32_t item_type;                // Item type ID (entry number in items.tbl)
    uint32_t respawn_time;             // Item respawn time in ms (0xFFFFFFFF if item is temporary)
    uint32_t count;                    // $Count or $Count Multi from items.tbl (not value from RFL)
    uint32_t item_handle;              // Item object ID
    uint16_t item_bit;                 // Item index in RF_ItemUpdatePacket, 0xFFFF if item is temporary (created after death)
    uint8_t unknown2;                  // 0x75 - enabled, 0x68 - not enabled
    struct RF_Vector pos;
    struct RF_Matrix rot_matrix;
#else
    char rest[];
#endif
};

struct RF_ReloadPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_RELOAD
    uint32_t entity_handle;            // Entity handle
    uint32_t weapon;                   // Entity weapon type
    uint32_t clip_ammo;                // Ammunition left in magazines
    uint32_t ammo;                     // Ammunition in current magazine
};

struct RF_ReloadRequestPacket
{
    struct RF_GamePacketHeader header; // RF_RELOAD_REQUEST
    uint32_t weapon;                   // Player weapon
};

enum RF_WeaponFireFlags
{
    RF_WFF_ALT_FIRE   = 0x01,
    RF_WFF_UNKNOWN    = 0x02,
    RF_WFF_NO_POS_ROT = 0x04,
};

struct RF_WeaponFirePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_WEAPON_FIRE
    uint8_t weapon;                    // Weapon type (see RF_Weapon)
    uint8_t flags;                     // Bitfield (RF_WeaponFireFlags)
#ifdef PSEUDOCODE
    if (is_client) {                   // exists only in packets sent by server
        uint8_t player_id;             // Player ID
    }
    if ((flags & RF_WFF_NO_POS_ROT) == 0) {
        struct RF_Vector pos;          // Position coordinate of weapon
        // unsigned?
        int16_t direction_x;           // X coordinate of direction vector
        int16_t direction_y;           // Y coordinate of direction vector
        int16_t direction_z;           // Z coordinate of direction vector
    }
    if (flags & RF_WFF_UNKNOWN) {
        uint8_t unknown;
    }
#else
    char rest[];
#endif
};

struct RF_FallDamagePacket
{
    struct RF_GamePacketHeader header; // RF_GPT_FALL_DAMAGE
    float force;                       // Force?
};

struct RF_SoundPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_SOUND
    uint16_t sound_id;                 // Sound ID from sounds.tbl
    struct RF_Vector pos;              // NaN causes the sound to be position independent in RF 1.20
};

struct RF_TeamsScoresPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_TEAM_SCORES
    uint16_t score_red;                // Score of red team
    uint16_t score_blue;               // Score of blue team
};

struct RF_GlassKillPacket
{
    struct RF_GamePacketHeader header; // RF_GPT_GLASS_KILL
    int32_t room_id;                   // 0x7FFFFFFF - index starting from 1
    uint8_t explosion;                 // 1 or 0
    struct RF_Vector pos;              // destruction start pos
    float unknown[3];                  // 0.0f if explosion == 1
};


/**
 * Packet union
 * Represents each normal game packet
 */
union RF_Packet
{
    struct RF_GamePacketHeader header;
    struct RF_GameInfoPacket game_info;
    struct RF_JoinRequestPacket join_request;
    struct RF_JoinAcceptPacket join_accept;
    struct RF_JoinDenyPacket join_deny;
    struct RF_NewPlayerPacket new_player;
    struct RF_PlayersPacket players;
    struct RF_LeftGamePacket left_game;
    struct RF_StateInfoRequestPacket state_info_request;
    struct RF_ChatLinePacket chat_line;
    struct RF_NameChangePacket name_change;
    struct RF_RespawnRequestPacket respawn_request;
    struct RF_LeaveLimboPacket leave_limbo;
    struct RF_TeamChangePacket team_change;
    struct RF_NetgameUpdatePacket netgame_update;
    struct RF_CtfFlagPickedUpPacket ctf_flag_picked_up;
    struct RF_CtfFlagCapturedPacket ctf_flag_captured;
    struct RF_CtfFlagUpdatePacket ctf_flags_update;
    struct RF_CtfFlagReturnedPacket ctf_flag_returned;
    struct RF_ObjectUpdatePacket object_update;
    struct RF_ObjectKillPacket object_kill;
    struct RF_EntityCreatePacket entity_create;
    struct RF_ReloadRequestPacket reload_request;
    struct RF_WeaponFirePacket weapon_fire;
    struct RF_TeamsScoresPacket teams_scores;
};

/**
 * @page Tracker packets
 */

enum RF_TrackerPacketType
{
    RF_TPT_REPLY               = 0x01,
    RF_TPT_SERVER_STOPPED      = 0x03,
    RF_TPT_SERVER_PING         = 0x04,
    RF_TPT_SERVER_LIST_REQUEST = 0x05,
    RF_TPT_SERVER_LIST         = 0x06,
    RF_TPT_SERVER_LIST_END     = 0x07,
};

struct RF_TrackerHeader
{
    uint8_t unknown;     // 0x06
    uint16_t type;
    uint32_t seq;
    uint16_t packet_len; // Length of whole packet (with packet class)
};

struct RF_TrackerServerAddress
{
    uint32_t ip;
    uint16_t port;
};

struct RF_TrackerServerList
{
    uint8_t unknown; // 0x06
    uint16_t type;   // RF_TPT_SERVER_LIST
    uint32_t seq;
    uint16_t packet_len;
    uint8_t packet_server_count;
    uint32_t server_count;
    struct RF_TrackerServerAddress servers[];
};

struct RF_TrackerUnknown
{
    uint8_t unknown; // 0x06
    uint16_t type;   // RF_TPT_SERVER_LIST
    uint32_t seq;
    uint16_t packet_len;
    uint8_t unknown2;
};

union RF_TrackerPacket
{
    struct RF_TrackerHeader header;
    struct RF_TrackerServerList server_list;
    struct RF_TrackerUnknown server_ping;
    struct RF_TrackerUnknown server_stop;
};

#pragma pack(pop)

#endif // _RFPROTO_H
