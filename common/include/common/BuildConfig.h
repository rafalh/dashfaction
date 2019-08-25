#pragma once

#ifdef HAS_PF
 #define MASK_AS_PF 1
#else
 #define MASK_AS_PF 0
#endif

#define NO_CD_FIX 1
#define SPECTATE_MODE_SHOW_WEAPON 1
#define WIDESCREEN_FIX 1
#define CHECK_PACKFILE_CHECKSUM 0 // slow (1 second on SSD on first load after boot)
#define D3D_SWAP_DISCARD 1 // needed for MSAA
#define D3D_LOCKABLE_BACKBUFFER 0
#define D3D_HW_VERTEX_PROCESSING 1
#define SERVER_WIN32_CONSOLE 1

#define CONSOLE_BG_R 0x00
#define CONSOLE_BG_G 0x00
#define CONSOLE_BG_B 0x40
#define CONSOLE_BG_A 0xC0
