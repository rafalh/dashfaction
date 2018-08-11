#pragma once

#ifdef HAS_PF
 #define MASK_AS_PF 1
#else
 #define MASK_AS_PF 0
#endif

#define NO_CD_FIX 1
#define NO_INTRO 1
#define LEVELS_AUTODOWNLOADER 1

#ifdef DEBUG
#define CAMERA_1_3_COMMANDS 1
#else
#define CAMERA_1_3_COMMANDS 0
#endif

#define SPECTATE_MODE_ENABLE 1
#define SPECTATE_MODE_SHOW_WEAPON 1
#define SPLITSCREEN_ENABLE 0
#define WINDOWED_MODE_SUPPORT 1
#define MULTISAMPLING_SUPPORT 1
#define ANISOTROPIC_FILTERING 1
#define DIRECTINPUT_SUPPORT 1
#define DIRECTINPUT_ENABLED 0
#define WIDESCREEN_FIX 1
#define CHECK_PACKFILE_CHECKSUM 0 // slow (1 second on SSD on first load after boot)
#define D3D_SWAP_DISCARD 1 // needed for MSAA
#define SERVER_WIN32_CONSOLE 0

#define CONSOLE_BG_R 0x00
#define CONSOLE_BG_G 0x00
#define CONSOLE_BG_B 0x40
#define CONSOLE_BG_A 0xC0
