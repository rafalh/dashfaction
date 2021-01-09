#pragma once

#ifdef HAS_PF
 #define MASK_AS_PF 1
#else
 #define MASK_AS_PF 0
#endif

#define NO_CD_FIX 1
#define SPECTATE_MODE_SHOW_WEAPON 1
#define D3D_SWAP_DISCARD 1 // needed for MSAA
#define D3D_LOCKABLE_BACKBUFFER 0
#define D3D_HW_VERTEX_PROCESSING 1
#define DEBUG_PERF 0
