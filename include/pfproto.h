#ifndef _PFPROTO_H
#define _PFPROTO_H

#include "rfproto.h"

#pragma pack(push, 1)

enum pfPacketType
{
	PF_CLIENT_INFO_REQUEST = 0x3B,
	PF_CLIENT_INFO = 0x3C,
	PF_CLIENT_INFO2_REQUEST = 0x3D,
	PF_CLIENT_INFO2 = 0x3E,
};

typedef struct _pfClientInfoRequest
{
	uint8_t type; /* PF_CLIENT_INFO_REQUEST */
    uint16_t size; /* 0x9 (broken) */
    uint16_t unknown; /* 02 02 (const) */
	uint32_t data;  /* 0B D2 7B ED  0xED7BD20B */
	                /* E6 EC CF C6  0xC6CFECE6 */
} pfClientInfoRequest;

typedef struct _pfClientInfo
{
	uint8_t type; /* PF_CLIENT_INFO */
    uint16_t size; /* 0xD (broken) */
    uint16_t unknown; /* 02 02 (const) */
    uint32_t data; /* (x + 0x4921511) ^ key, eg. 0x85A7B7B1, 0xB8039EC3, x = 0x3104D9C7, 0x16B0FEED */ 
    uint32_t key; /* g_PerformanceCount2 , eg. 0x9EE4A44F, 0x8D95701B */
} pfClientInfo;

typedef struct _pfClientInfo2Request
{
	uint8_t type; /* PF_CLIENT_INFO2_REQUEST */
    uint16_t size; /* 0xB (broken) */
    uint8_t unknown; /* 02 (const) */
    uint8_t unused; /* Eg. 40, 00 */
    uint32_t unknown2; /* Eg. 0x0232AB19, 0x2171AA64, x - pfClientInfo.key */
    uint8_t is_serv_modded; /* Eg. 00, 00, byte_1002178B */
    uint8_t unknown3; /* Eg. 01, 00, byte_10021753 */
} pfClientInfo2Request;

typedef struct _pfClientInfo2
{
	uint8_t type; /* PF_CLIENT_INFO2 */
    uint16_t size; /* 0x14 (broken) */
    uint8_t unknown; /* 02 (const) */
    uint32_t data; /* Eg. 0x4D308CFC, 0x5B205813 (diff DEFCB17) */
    uint32_t data2; /* Eg. 0xE1BB3DAC, 0xEFAB08C3 (diff DEFCB17) */
    uint32_t data3; /* Eg. 0x74A18CDE, 0x829157F5 (diff DEFCB17) */
    uint32_t data4; /* Eg. 0xA2184F69, 0xB0081A80 (diff DEFCB17) */
} pfClientInfo2;

#pragma pack(pop)

#endif /* _PFPROTO_H */
