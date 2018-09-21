#include "stdafx.h"
#include "rf.h"
#include "utils.h"
#include "commands.h"

using namespace rf;

#ifdef DEBUG

#if 0 // research bones

#pragma pack(push, 8)
namespace rf
{
    struct CQuaternion
    {
        float x, y, z, w;
    };
    struct CTransform
    {
        CMatrix3 matRot;
        CVector3 vOrgin;
    };
    struct CBone
    {
        char szName[24];
        CTransform Transform;
        int Parent;
    };
    struct SAnimatorElement
    {
        int iAnim;
        int AnimTime;
        float field_8;
    };
    struct UnkMatOrTransformForAnim
    {
        __int16 UpdateSeq;
        __int16 field_2;
        CMatrix3 field_4;
        int field_28;
        float field_2C;
    };
    struct CSubMeshInfo;
    struct CSubMesh;
    struct CColSphere;
    struct CMeshMaterial;
    struct CMesh
    {
        char szName[68];
        int Version;
        int cSubMesh;
        CSubMeshInfo *pSubMeshInfo;
        int cSubMeshes;
        CSubMesh *pSubMeshes;
        int field_58;
        int field_5C;
        int cColSpheres;
        CColSphere *pColSpheres;
        int field_68;
        int field_6C;
        int field_70;
        int field_74;
        int cTotalMaterials;
        CMeshMaterial *pAllMaterials;
        int field_80;
        int field_84;
        int field_88;
        int field_8C;
        CSubMeshInfo *pSubMeshInfo2;
    };
    struct CMvfAnim;
    struct CCharacterMesh
    {
        char szName[4];
        int field_4[16];
        int Flags;
        int cBones;
        CBone Bones[50];
        BYTE BoneIndexes[32];
        int field_F44;
        int field_F48;
        int field_F4C;
        int field_F50;
        int field_F54;
        int cAnims;
        CMvfAnim *Animations[16];
        int field_F9C[102];
        int field_1134[54];
        BYTE IsStateSkeleton[16];
        int field_121C[39];
        int field_12B8;
        int field_12BC;
        CTransform field_12C0;
        int field_12F0[280];
        int field_1750[9];
        int field_1774[50];
        int field_183C[50];
        int field_1904[20];
        int field_1954[10];
        int field_197C[10];
        int field_19A4[5];
        int field_19B8;
        int cMeshes;
        CMesh Meshes[1];
        int RootBoneIndex;
    };
    struct CCharacterAnimator
    {
        CTransform BoneTransformsCombined[50];
        CTransform BoneTransformsFinal[50];
        CVector3 vMoveRootBoneTemp12C0;
        int field_12CC;
        int cElements;
        SAnimatorElement Elements[16];
        UnkMatOrTransformForAnim BoneStates1394[50];
        int field_1CF4;
        __int16 UpdateSeq;
        __int16 field_1CFA;
        int field_1CFC;
        int field_1D00;
        float field_1D04;
        CVector3 field_1D08;
        int field_1D14;
        int field_1D18;
        int field_1D1C;
        CVector3 field_1D20;
        CVector3 field_1D2C;
        CVector3 field_1D38;
        int field_1D44;
        int field_1D48;
        int field_1D4C;
        CCharacterMesh *pCharacter;
        int field_1D54;
        int field_1D58;
    };
}
#pragma pack(pop)

using namespace rf;

static const auto SortBonesByLevel = (void(*)(BYTE *pResult, int cBones, CBone *pBones))0x0051CB50;
static const auto Transform_Mul = (CTransform*(__thiscall*)(CTransform *This, CTransform *pResult, CTransform *pParentTransform))0x0051C620;
static const auto CAnimMesh_UpdateTime = (void(__thiscall*)(CAnimMesh *This, float fDeltaTime, int a3, CVector3 *a4, CMatrix3 *a5, int a6))0x00501AB0;
static const auto CharacterAnimatorUpdateBones = (void(*)(int NumBones, unsigned __int8 *BoneBranchIndices, CCharacterAnimator *pCharAnim))0x0051B500;
static const auto MvfAnim_CalcBonePos = (CVector3*(__thiscall*)(CMvfAnim *This, CVector3 *pResult, int BoneId, int AnimTime))0x0053A130;
static const auto MvfAnim_CalcBoneRot = (CQuaternion*(__thiscall*)(CMvfAnim *This, CQuaternion *pResult, int BoneId, int AnimTime))0x00539ED0;

#define BONE_NAME "park-bdbn-lowerleg-l"
//#define BONE_NAME "park-bdbn-root"

int g_TestBoneIdx = -1;

void PrintTransform(CTransform *pTransform)
{
    ERR("origin %f %f %f", pTransform->vOrgin.x, pTransform->vOrgin.y, pTransform->vOrgin.z);
    for (int i = 0; i < 3; ++i)
        ERR("mat[%d] %f %f %f", i, pTransform->matRot.rows[i].x, pTransform->matRot.rows[i].y, pTransform->matRot.rows[i].z);
}

void SortBonesByLevelHook(BYTE *pResult, int cBones, CBone *pBones)
{
    for (int i = 0; i < cBones; ++i)
    {
        if (!strcmp(pBones[i].szName, BONE_NAME))
        {
            g_TestBoneIdx = i;
            ERR("Bone %d - %s", i, pBones[i].szName);
            ERR("trans");
            PrintTransform(&pBones[i].Transform);

            ERR("trans^2");
            CTransform temp;
            Transform_Mul(&pBones[i].Transform, &temp, &pBones[i].Transform);
            PrintTransform(&temp);

            //pBones[i].Transform.vOrgin.x = (rand() % 1000) / 1000.0f;
            //pBones[i].Transform.vOrgin.y = (rand() % 1000) / 1000.0f;
            //pBones[i].Transform.vOrgin.z = (rand() % 1000) / 1000.0f;

            
        }
    }
    SortBonesByLevel(pResult, cBones, pBones);
}

void AnimMeshUpdateTimeHook(CAnimMesh *pAnimMesh, float fDeltaTime, int a3, CVector3 *a4, CMatrix3 *a5, int bUnk)
{
    CAnimMesh_UpdateTime(pAnimMesh, 0.0f, a3, a4, a5, bUnk);
}

void CharacterAnimatorUpdateBonesHook(int NumBones, unsigned __int8 *BoneBranchIndices, CCharacterAnimator *pCharAnim)
{
    if (!strcmp(pCharAnim->pCharacter->szName, "miner") && NumBones > 10)
    {
        static int cnt = 0;
        if (cnt++ == 10)
        {
            ERR("mesh %s NumBones %d", pCharAnim->pCharacter->szName, NumBones);
            ERR("vMoveRootBoneTemp12C0 %f %f %f", pCharAnim->vMoveRootBoneTemp12C0.x, pCharAnim->vMoveRootBoneTemp12C0.y, pCharAnim->vMoveRootBoneTemp12C0.z);
        }
    }

    CharacterAnimatorUpdateBones(NumBones, BoneBranchIndices, pCharAnim);

    if (!strcmp(pCharAnim->pCharacter->szName, "miner") && NumBones > 10)
    {
        static int cnt = 0;
        if (cnt++ == 10)
        {
            int BoneId = g_TestBoneIdx; // park-bdbn-lowerleg-l
            ERR("vMoveRootBoneTemp12C0 %f %f %f", pCharAnim->vMoveRootBoneTemp12C0.x, pCharAnim->vMoveRootBoneTemp12C0.y, pCharAnim->vMoveRootBoneTemp12C0.z);
            ERR("BoneTransformsCombined");
            PrintTransform(&pCharAnim->BoneTransformsCombined[BoneId]);
            ERR("BoneTransformsFinal");
            PrintTransform(&pCharAnim->BoneTransformsFinal[BoneId]);
            ERR("CombinedUpdateSeq %d FinalUpdateSeq %d field_28 %d field_2C %f field_4[0] %f %f %f",
                pCharAnim->BoneStates1394[BoneId].UpdateSeq, pCharAnim->BoneStates1394[BoneId].field_2, 
                pCharAnim->BoneStates1394[BoneId].field_28, pCharAnim->BoneStates1394[BoneId].field_2C,
                pCharAnim->BoneStates1394[BoneId].field_4.rows[0].x, pCharAnim->BoneStates1394[BoneId].field_4.rows[0].y, pCharAnim->BoneStates1394[BoneId].field_4.rows[0].z);

            //for (int i = 0; i < pCharAnim->pCharacter->cAnims; ++i)
            //    ERR("%d %s", i, pCharAnim->pCharacter->Animations[i]);
            int iAnim = 0; // ult2_stand.mvf
            CVector3 BonePos;
            CQuaternion BoneRot;
            ERR("Anim %s", pCharAnim->pCharacter->Animations[iAnim]);
            MvfAnim_CalcBonePos(pCharAnim->pCharacter->Animations[iAnim], &BonePos, g_TestBoneIdx, 0);
            MvfAnim_CalcBoneRot(pCharAnim->pCharacter->Animations[iAnim], &BoneRot, g_TestBoneIdx, 0);
            ERR("Bone pos %f %f %f", BonePos.x, BonePos.y, BonePos.z);
            ERR("Bone rot %f %f %f %f", BoneRot.x, BoneRot.y, BoneRot.z, BoneRot.w);
        }
    }
}

auto CharacterAnimatorBuildTransform = (void(*)(CTransform *pOutTransform, CQuaternion *pRots, CVector3 *pPositions, int Num, float *a5))0x0051B110;

void CharacterAnimatorBuildTransformHook(CTransform *pOutTransform, CQuaternion *pRots, CVector3 *pPositions, int Num, float *a5)
{
    CharacterAnimatorBuildTransform(pOutTransform, pRots, pPositions, Num, a5);
    //ERR("CharacterAnimatorBuildTransform Num %d", Num);
}

#endif

#if 0
static void RenderEntityHook(EntityObj *pEntity)
{ // TODO
    DWORD dwOldLightining;
    GrFlushBuffers();
    IDirect3DDevice8_GetRenderState(*g_ppGrDevice, D3DRS_LIGHTING, &dwOldLightining);
    IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_LIGHTING, 1);
    IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_AMBIENT, 0xFFFFFFFF);

    RfRenderEntity((CObject*)pEntity);
    GrFlushBuffers();
    //IDirect3DDevice8_SetRenderState(*g_ppGrDevice, D3DRS_LIGHTING, dwOldLightining);
}
#endif

static void TestCmdHandler(void)
{
    if (*rf::g_pbDcRun)
    {
        //RfCmdGetNextArg(CMD_ARG_FLOAT, 0);
        //*((float*)0x017C7BD8) = *g_pfCmdArg;

        //int *ptr = (int*)HeapAlloc(GetProcessHeap(), 0, 8);
        //ptr[10] = 1;
        //DcPrintf("test done %p", ptr);
    }

    if (*rf::g_pbDcHelp)
        rf::DcPrintf("     test <n>", NULL);
}

static rf::DcCommand TestCmd = { "test", "Test command", TestCmdHandler };

static const auto GetMousePos = (int(*)(int *pX, int *pY, int *pZ))0x0051E450;

bool g_InSetViewForScene = false;

void GrSetViewMatrixHook(CMatrix3 *pMatRot, CVector3 *pPos, float Fov, int bClearZBuffer, int a5)
{
    D3DRECT rc[2] = { { 0, 0, 1680, 1 },{ 0, 0, 1, 1050 } };
    //(*rf::g_ppGrDevice)->Clear(1, &rc[1], D3DCLEAR_TARGET, 0xFF0000FF, 1.0f, 0);
    g_InSetViewForScene = true;
    GrSetViewMatrix(pMatRot, pPos, Fov, bClearZBuffer, a5);
    g_InSetViewForScene = false;
    //(*rf::g_ppGrDevice)->Clear(1, &rc[1], D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
}

void Test_GrClearZBuffer_SetRect()
{
    if (g_InSetViewForScene)
    {
        //(*rf::g_ppGrDevice)->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0xFF00FF00, 1.0f, 0);

        D3DRECT rc[2] = { { 0, 0, 1680, 1 },{ 0, 0, 1, 1050 } };
        /*(*rf::g_ppGrDevice)->Clear(0, NULL, D3DCLEAR_TARGET, 0xFFFF0000, 1.0f, 0);
        (*rf::g_ppGrDevice)->Clear(1, &rc, D3DCLEAR_TARGET| D3DCLEAR_ZBUFFER, 0xFFFF0000, 1.0f, 0);*/
        //(*rf::g_ppGrDevice)->Clear(1, &rc[1], D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
        //GrSetColor(0, 255, 0, 255);
        //GrDrawRect(0, 0, g_pGrScreen->MaxWidth, g_pGrScreen->MaxHeight, *g_pGrRectMaterial);
        //g_pGrScreen->field_8C = 1;
    }

    *(int*)0x00637090 = 1;
}

void FogClearHook()
{
    (*rf::g_ppGrDevice)->Clear(0, NULL, D3DCLEAR_TARGET, 0xFFFF0000, 1.0f, 0);
    
#if 0
    *(float*)0x01818B58 += 0.5f; // g_fGrHalfViewportWidth_0
    *(float*)0x01818B60 -= 0.5f; // g_fGrHalfViewportHeightNeg
    *(float*)0x01818A5C += 0.5f; // g_fGrHalfViewportWidth
    *(float*)0x01818A24 += 0.5f; // g_fGrHalfViewportHeight
#endif

    // Left and top edge fix for MSAA (RF does similar thing in GrDrawTextureD3D)
    static float OffsetGeomX = -0.5;
    static float OffsetGeomY = -0.5;
    OffsetGeomX = g_pGrScreen->OffsetX - 0.5f;
    OffsetGeomY = g_pGrScreen->OffsetY - 0.5f;
    *(float*)0x01818B54 -= 0.5f; // viewport center x
    *(float*)0x01818B5C -= 0.5f; // viewport center y

    WriteMemUInt8(0x005478C6, ASM_FADD);
    WriteMemUInt8(0x005478D7, ASM_FADD);
    WriteMemPtr(0x005478C6 + 2, &OffsetGeomX);
    WriteMemPtr(0x005478D7 + 2, &OffsetGeomY);
}


void UiButton_BmGetBitmapSizeHook(int BmHandle, int *pWidth, int *pHeight)
{
    struct
    {
        const char *name;
        int w, h;
    } ButtonSize[] = {
        { "button.tga", 204, 61 },
        { "button_more.tga", 204, 61 },
        { "cyclebutton_l.tga", 19, 26 },
        { "cyclebutton_r.tga", 19, 26 },
        { "cyclebutton_u.tga", 21, 18 },
        { "cyclebutton_d.tga", 21, 18 },
        { "arrow_up_long.tga", 102, 20 },
        { "arrow_down_long.tga", 110, 20 },
        { "main_title.vbm", 512, 366 },
        { "indicator.tga", 14, 28 },
    };

    BmGetBitmapSize(BmHandle, pWidth, pHeight);

    const char *Filename = BmGetFilename(BmHandle);
    for (unsigned i = 0; i < COUNTOF(ButtonSize); ++i)
    {
        if (!strcmp(Filename, ButtonSize[i].name))
        {
            *pWidth = ButtonSize[i].w;
            *pHeight = ButtonSize[i].h;
            return;
        }
    }

    ERR("Unknown UiButton texture: %s %d %d", Filename, *pWidth, *pHeight);
}

bool ConvertBitmapFormat(BYTE *pDstBits, BmPixelFormat DstPixelFmt, const BYTE *pSrcBits, BmPixelFormat SrcPixelFmt, int Width, int Height, int DstPitch, int SrcPitch)
{
    if (DstPixelFmt != BMPF_8888) return false;
    switch (SrcPixelFmt)
    {
    case BMPF_888:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
            {
                *(pDstPtr++) = *(pSrcPtr++);
                *(pDstPtr++) = *(pSrcPtr++);
                *(pDstPtr++) = *(pSrcPtr++);
                *(pDstPtr++) = 255;
            }
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    case BMPF_4444:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
            {
                *(pDstPtr++) = (*(pSrcPtr) & 0x0F) * 17;
                *(pDstPtr++) = ((*(pSrcPtr++) & 0xF0) >> 4) * 17;
                *(pDstPtr++) = (*(pSrcPtr) & 0x0F) * 17;
                *(pDstPtr++) = ((*(pSrcPtr++) & 0xF0) >> 4) * 17;
            }
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    case BMPF_1555:
        for (int y = 0; y < Height; ++y)
        {
            BYTE *pDstPtr = pDstBits;
            const BYTE *pSrcPtr = pSrcBits;
            for (int x = 0; x < Width; ++x)
            {
                WORD SrcWord = *(WORD*)pSrcPtr;
                pSrcPtr += 2;
                *(pDstPtr++) = ((SrcWord & (0x1F << 0)) >> 0) * 255 / 31;
                *(pDstPtr++) = ((SrcWord & (0x1F << 5)) >> 5) * 255 / 31;
                *(pDstPtr++) = ((SrcWord & (0x1F << 10)) >> 10) * 255 / 31;
                *(pDstPtr++) = (SrcWord & 0x8000) ? 255 : 0;
            }
            pDstBits += DstPitch;
            pSrcBits += SrcPitch;
        }
        return true;
    default:
        return false;
    }
}

int GetPixelFormatFromD3DFormat(int PixelFormat, int *BytesPerPixel)
{
    static int Dummy;
    if (!BytesPerPixel)
        BytesPerPixel = &Dummy;
    switch (PixelFormat)
    {
    case D3DFMT_R5G6B5:
        *BytesPerPixel = 2;
        return BMPF_565;
    case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4:
        *BytesPerPixel = 2;
        return BMPF_4444;
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
        *BytesPerPixel = 2;
        return BMPF_1555;
    case D3DFMT_R8G8B8:
        *BytesPerPixel = 3;
        return BMPF_888;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        *BytesPerPixel = 4;
        return BMPF_8888;
    default:
        return 0;
    }
}

int GrD3DSetTextureDataHook(int Level, const BYTE *pSrcBits, const BYTE *pPallete, int cxBm, int cyBm, int PixelFmt, void *a7, int cxTex, int cyTex, IDirect3DTexture8 *pTexture)
{
    D3DLOCKED_RECT LockedRect;
    HRESULT hr = pTexture->LockRect(Level, &LockedRect, 0, 0);
    if (FAILED(hr))
    {
        ERR("LockRect failed");
        return -1;
    }
    D3DSURFACE_DESC Desc;
    pTexture->GetLevelDesc(Level, &Desc);

    bool bUseFallback = false;
    int Result = 0;

    if (Desc.Format == D3DFMT_A8R8G8B8)
    {
        const BYTE *pSrcPtr = (const BYTE*)pSrcBits;
        BYTE *pDstPtr = (BYTE*)LockedRect.pBits;
        for (int y = 0; y < cyBm; ++y)
        {
            BYTE *pRowPtr = pDstPtr;
            for (int x = 0; x < cxBm; ++x)
            {
                if (PixelFmt == BMPF_8888)
                {
                    *(pDstPtr++) = *(pSrcPtr++); // b
                    *(pDstPtr++) = *(pSrcPtr++); // g
                    *(pDstPtr++) = *(pSrcPtr++); // r
                    *(pDstPtr++) = *(pSrcPtr++); // a
                }
                else if (PixelFmt == BMPF_4444)
                {
                    *(pDstPtr++) = (*(pSrcPtr) & 0x0F) * 17;
                    *(pDstPtr++) = ((*(pSrcPtr++) & 0xF0) >> 4) * 17;
                    *(pDstPtr++) = (*(pSrcPtr) & 0x0F) * 17;
                    *(pDstPtr++) = ((*(pSrcPtr++) & 0xF0) >> 4) * 17;
                }
                else if (PixelFmt == BMPF_1555)
                {
                    WORD SrcWord = *(WORD*)pSrcPtr;
                    pSrcPtr += 2;
                    *(pDstPtr++) = ((SrcWord & (0x1F << 0)) >> 0) * 255 / 31;
                    *(pDstPtr++) = ((SrcWord & (0x1F << 5)) >> 5) * 255 / 31;
                    *(pDstPtr++) = ((SrcWord & (0x1F << 10)) >> 10) * 255 / 31;
                    *(pDstPtr++) = (SrcWord & 0x8000) ? 255 : 0;
                }
                else
                {
                    ERR("Unsupported pixel format %d", PixelFmt);
                    Result = -1;
                    goto ExitLoops;
                }
            }
            pDstPtr = pRowPtr + LockedRect.Pitch;
        }
    }
    else
        bUseFallback = true;
ExitLoops:
    pTexture->UnlockRect(Level);
    if (!bUseFallback)
        return Result;

    return GrD3DSetTextureData(Level, pSrcBits, pPallete, cxBm, cyBm, PixelFmt, a7, cxTex, cyTex, pTexture);
}

#include <MemUtils.h>

void TestInit()
{
#if 0
    WriteMemInt32(0x0051CB3B + 1, (uintptr_t)SortBonesByLevelHook - (0x0051CB3B + 0x5));
    //WriteMemUInt8(0x0051BA80, ASM_RET);

    WriteMemUInt8(0x00503360, ASM_LONG_JMP_REL);
    WriteMemInt32(0x00503360 + 1, (uintptr_t)AnimMeshUpdateTimeHook - (0x00503360 + 0x5));

    WriteMemInt32(0x0051BA1A + 1, (uintptr_t)CharacterAnimatorUpdateBonesHook - (0x0051BA1A + 0x5));

    WriteMemInt32(0x0051B81F + 1, (uintptr_t)CharacterAnimatorBuildTransformHook - (0x0051B81F + 0x5));
#endif
    CommandRegister(&TestCmd);

    // Change no-focus sleep argument
    WriteMemInt32(0x004353D3 + 1, 100);

    //WriteMemUInt32(0x00488BE9, ((uintptr_t)RenderEntityHook) - (0x00488BE8 + 0x5));

    // menu buttons speed
    //WriteMemFloat(0x00598FC0, 0.5f);
    //WriteMemFloat(0x00598FC4, -0.5f);
    
#if 0 // 32 bit texture formats
    // default: D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, D3DFMT_A4R4G4B4, D3DFMT_A8R3G3B2
    // Note: RF code is broken for 32-bit formats. We can fix it to have better alpha textures (only 4 bits per channel is super small)
    // What will change - menu should look better except VBM files - they dont support 32-bit colors
    WriteMemUInt32(0x005A7DFC, D3DFMT_R8G8B8);
    WriteMemUInt32(0x005A7E00, D3DFMT_R5G6B5);
    WriteMemUInt32(0x005A7E04, D3DFMT_A8R8G8B8); // lightmaps
    WriteMemUInt32(0x005A7E08, D3DFMT_A8R8G8B8);
    WriteMemUInt32(0x005A7E0C, D3DFMT_A4R4G4B4);

    WriteMemInt32(0x0055B80E + 1, (uintptr_t)GrD3DSetTextureDataHook - (0x0055B80E + 0x5));
    WriteMemInt32(0x0055CA0B + 1, (uintptr_t)GrD3DSetTextureDataHook - (0x0055CA0B + 0x5));

    WriteMemUInt8(0x004ED3F5, ASM_PUSH_EBX);
    WriteMemInt32(0x004ED3F6 + 1, (uintptr_t)RflLoadLightmaps_004ED3F6 - (0x004ED3F6 + 0x5));
#endif

#if 0 // FIXME: rocket launcher screen is broken
    // Improved Railgun Scanner resolution
    const int8_t RailResolution = 120; // default is 64, max is 127 (signed byte)
    WriteMemUInt8(0x004AE0B7 + 1, RailResolution);
    WriteMemUInt8(0x004AE0B9 + 1, RailResolution);
    WriteMemUInt8(0x004ADD70 + 1, RailResolution);
    WriteMemUInt8(0x004ADD72 + 1, RailResolution);
    WriteMemUInt8(0x004A34BB + 1, RailResolution);
    WriteMemUInt8(0x004A34BD + 1, RailResolution);
    WriteMemUInt8(0x004325E6 + 1, RailResolution);
    WriteMemUInt8(0x004325E8 + 1, RailResolution);
#endif

#if 0 // tests for edge fixes
    // fog test
    WriteMemInt32(0x00431EA5 + 1, (uintptr_t)FogClearHook - (0x00431EA5 + 0x5));
    WriteMemUInt8(0x0043297A, ASM_NOP, 5);

    WriteMemInt32(0x00431D14 + 1, (uintptr_t)GrSetViewMatrixHook - (0x00431D14 + 0x5));
   
#endif

    WriteMemInt32(0x00457525 + 1, (uintptr_t)UiButton_BmGetBitmapSizeHook - (0x00457525 + 0x5));

#if 0
    // Bink
    WriteMemUInt8(0x00520AE0, ASM_NOP, 0x00520AED - 0x00520AE0);
    WriteMemUInt8(0x00520AF9, ASM_NOP, 0x00520B06 - 0x00520AF9);
#endif
}

void TestRenderInGame()
{

}

void TestInitAfterGame()
{

}

void TestRender()
{
    char Buffer[256];
    int x = 50, y = 200;
    EntityObj *pEntity = rf::EntityGetFromHandle((*g_ppLocalPlayer)->hEntity);
    rf::GrSetColor(255, 255, 255, 255);
    //MemReference<float, 0x005A4014> FrameTime;
    auto &FrameTime = *(float*)0x005A4014;

    if (pEntity)
    {
        sprintf(Buffer, "Flags 0x%X", pEntity->_Super.Flags);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
        sprintf(Buffer, "PhysInfo Flags 0x%X", pEntity->_Super.PhysInfo.Flags);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
        sprintf(Buffer, "MovementMode 0x%X", pEntity->MovementMode->Id);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
        sprintf(Buffer, "pEntity 0x%p", pEntity);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
        sprintf(Buffer, "framerate %f", FrameTime);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
        static const auto *g_pAppTimeMs = (int*)0x5A3ED8;
        sprintf(Buffer, "AppTimeMs %d", *g_pAppTimeMs);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
        sprintf(Buffer, "WeaponClsId %d", pEntity->WeaponInfo.WeaponClsId);
        GrDrawText(x, y, Buffer, -1, *rf::g_pGrTextMaterial);
        y += 15;
    }
}

#endif // DEBUG
