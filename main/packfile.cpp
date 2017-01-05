#include "stdafx.h"
#include "packfile.h"
#include "utils.h"
#include "rf.h"

typedef struct _VFS_LOOKUP_TABLE_NEW
{
    struct _VFS_LOOKUP_TABLE_NEW *pNext;
    PACKFILE_ENTRY *pPackfileEntry;
} VFS_LOOKUP_TABLE_NEW;

static unsigned g_cPackfiles = 0;
static PACKFILE **g_pPackfiles = NULL;
static VFS_LOOKUP_TABLE_NEW *g_pVfsLookupTableNew = (VFS_LOOKUP_TABLE_NEW*)0x01BB2AC8; // g_pVfsLookupTable
static BOOL g_bTest = FALSE;
static unsigned g_cFilesInVfs = 0;

#define DEBUG_VFS 1
//#define DEBUG_VFS_FILENAME1 "Flame01_02.tga"
//#define DEBUG_VFS_FILENAME2 "big_glassblast_a.tga"

#define VFS_DBGPRINT TRACE

static EGameLang DetectInstalledGameLang()
{
    char szFullPath[MAX_PATH];
    const char *LangCodes[] = { "en", "gr", "fr" };
    for (unsigned i = 0; i < COUNTOF(LangCodes); ++i)
    {
        sprintf(szFullPath, "%s/maps_%s.vpp", g_pszRootPath, LangCodes[i]);
        FILE *pFile = fopen(szFullPath, "rb");
        if (pFile)
        {
            fclose(pFile);
            return (EGameLang)i;
        }
    }
    WARN("Cannot detect game language");
    return LANG_EN; // default
}

EGameLang GetInstalledGameLang()
{
    static bool Initialized = false;
    static EGameLang InstalledGameLang;
    if (!Initialized)
        InstalledGameLang = DetectInstalledGameLang();
    return InstalledGameLang;
}

static BOOL VfsLoadPackfileHook(const char *pszFilename, const char *pszDir)
{
    char szFullPath[256], Buf[0x800];
    FILE *pFile;
    PACKFILE *pPackfile;
    BOOL bRet = FALSE;
    unsigned i, cFilesInBlock, cAdded, OffsetInBlocks;
    
    VFS_DBGPRINT("Load packfile %s %s", pszDir, pszFilename);
    
    sprintf(szFullPath, "%s%s%s", g_pszRootPath, pszDir ? pszDir : "", pszFilename);
    if(!pszFilename || strlen(pszFilename) > 0x1F || strlen(szFullPath) > 0x7F)
        return FALSE;
    
    for(i = 0; i < g_cPackfiles; ++i)
        if(!stricmp(g_pPackfiles[i]->szPath, szFullPath))
            return TRUE;
    
    pFile = fopen(szFullPath, "rb");
    if(!pFile)
    {
        ERR("Failed to load vpp %s", szFullPath);
        return FALSE;
    }
    
    if(g_cPackfiles % 64 == 0)
    {
        g_pPackfiles = (PACKFILE**)realloc(g_pPackfiles, (g_cPackfiles + 64) * sizeof(PACKFILE*));
        memset(&g_pPackfiles[g_cPackfiles], 0, 64 * sizeof(PACKFILE*));
    }
    
    pPackfile = (PACKFILE*)malloc(sizeof(*pPackfile));
    if(!pPackfile)
    {
        ERR("malloc failed");
        return FALSE;
    }
    
    strncpy(pPackfile->szName, pszFilename, 0x1F);
    strncpy(pPackfile->szPath, szFullPath, 0x7F);
    pPackfile->field_A0 = 0;
    pPackfile->cFiles = 0;
    pPackfile->pFileList = NULL;
    
    if(fread(Buf, sizeof(Buf), 1, pFile) == 1 &&
       VfsProcessPackfileHeader(pPackfile, Buf))
    {
        pPackfile->pFileList = (PACKFILE_ENTRY*)malloc(pPackfile->cFiles * sizeof(PACKFILE_ENTRY));
        memset(pPackfile->pFileList, 0, pPackfile->cFiles * sizeof(PACKFILE_ENTRY));
        cAdded = 0;
        OffsetInBlocks = 1;
        bRet = TRUE;
        
        for(i = 0; i < pPackfile->cFiles; i += 32)
        {
            if(fread(Buf, sizeof(Buf), 1, pFile) != 1)
            {
                bRet = FALSE;
                ERR("Failed to fread vpp %s", szFullPath);
                break;
            }
            
            cFilesInBlock = min(pPackfile->cFiles - i, 32);
            VfsAddPackfileEntries(pPackfile, Buf, cFilesInBlock, &cAdded);
            ++OffsetInBlocks;
        }
    } else ERR("Failed to fread vpp 2 %s", szFullPath);
    
    if(bRet)
        VfsSetupFileOffsets(pPackfile, OffsetInBlocks);
    
    fclose(pFile);
    
    if(bRet)
    {
        g_pPackfiles[g_cPackfiles] = pPackfile;
        ++g_cPackfiles;
    } else {
        free(pPackfile->pFileList);
        free(pPackfile);
    }
    
    return bRet;
}

static PACKFILE *VfsFindPackfileHook(const char *pszFilename)
{
    unsigned i;
    
    for(i = 0; i < g_cPackfiles; ++i)
    {
        if(!stricmp(g_pPackfiles[i]->szName, pszFilename))
            return g_pPackfiles[i];
    }
    
    ERR("Packfile %s not found", pszFilename);
    return NULL;
}

static BOOL VfsBuildPackfileEntriesListHook(const char *pszExtList, char **ppFilenames, unsigned *pcFiles, const char *pszPackfileName)
{
    char *pszExt, *BufPtr;
    unsigned i, j, cbBuf = 1;
    PACKFILE *pPackfile;
    
    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    *pcFiles = 0;
    *ppFilenames = 0;
    
    for(i = 0; i < g_cPackfiles; ++i)
    {
        pPackfile = g_pPackfiles[i];
        if(!pszPackfileName || !stricmp(pszPackfileName, pPackfile->szName))
        {
            for(j = 0; j < pPackfile->cFiles; ++j)
            {
                pszExt = GetFileExt(pPackfile->pFileList[j].pszFileName);
                if(pszExt[0] && strstr(pszExtList, pszExt + 1))
                    cbBuf += strlen(pPackfile->pFileList[j].pszFileName) + 1;
            }
        }
    }
    
    *ppFilenames = (char*)RfMalloc(cbBuf);
    if(!*ppFilenames)
        return FALSE;
    BufPtr = *ppFilenames;
    for(i = 0; i < g_cPackfiles; ++i)
    {
        pPackfile = g_pPackfiles[i];
        if(!pszPackfileName || !stricmp(pszPackfileName, pPackfile->szName))
        {
            for(j = 0; j < pPackfile->cFiles; ++j)
            {
                pszExt = GetFileExt(pPackfile->pFileList[j].pszFileName);
                if(pszExt[0] && strstr(pszExtList, pszExt + 1))
                {
                    strcpy(BufPtr, pPackfile->pFileList[j].pszFileName);
                    BufPtr += strlen(pPackfile->pFileList[j].pszFileName) + 1;
                    ++(*pcFiles);
                }
            }
            
        }
    }
    
    return TRUE;
}

static BOOL VfsAddPackfileEntriesHook(PACKFILE *pPackfile, const void *pBlock, unsigned cFilesInBlock, unsigned *pcAddedFiles)
{
    unsigned i;
    PACKFILE_ENTRY *pEntry;
    BYTE *pData = (BYTE*)pBlock;
    
    for(i = 0; i < cFilesInBlock; ++i)
    {
        pEntry = &pPackfile->pFileList[*pcAddedFiles];
        if(*g_pbVfsIgnoreTblFiles && !stricmp(GetFileExt((char*)pData), ".tbl"))
            pEntry->pszFileName = "DEADBEEF";
        else
        {
            pEntry->pszFileName = (char*)malloc(strlen((char*)pData) + 10);
            memset(pEntry->pszFileName, 0, strlen((char*)pData) + 10);
            strcpy(pEntry->pszFileName, (char*)pData);
        }
        pEntry->dwNameChecksum = VfsCalcFileNameChecksum(pEntry->pszFileName);
        pEntry->cbFileSize = *((uint32_t*)(pData + 60));
        pEntry->pArchive = pPackfile;
        pEntry->pRawFile = NULL;
        
        pData += 64;
        ++(*pcAddedFiles);
        
        VfsAddFileToLookupTable(pEntry);
        ++g_cFilesInVfs;
    }
    return TRUE;
}

static void VfsAddFileToLookupTableHook(PACKFILE_ENTRY *pEntry)
{
    VFS_LOOKUP_TABLE_NEW *pLookupTableItem;
    
    pLookupTableItem = &g_pVfsLookupTableNew[pEntry->dwNameChecksum % 20713];
    
    while(TRUE)
    {
        if(!pLookupTableItem->pPackfileEntry)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME1) || !stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 1: %s (%x)", pEntry->pszFileName, pEntry->dwNameChecksum);
#endif
                
            break;
        }
            
        if(!stricmp(pLookupTableItem->pPackfileEntry->pszFileName, pEntry->pszFileName))
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME1) || !stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 2: %s (%x)", pEntry->pszFileName, pEntry->dwNameChecksum);
#endif
            break;
        }
            
        if(!pLookupTableItem->pNext)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME1) || !stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 3: %s (%x)", pEntry->pszFileName, pEntry->dwNameChecksum);
#endif
            
            pLookupTableItem->pNext = (VFS_LOOKUP_TABLE_NEW*)malloc(sizeof(VFS_LOOKUP_TABLE_NEW));
            pLookupTableItem = pLookupTableItem->pNext;
            pLookupTableItem->pNext = NULL;
            break;
        }
        pLookupTableItem = pLookupTableItem->pNext;
    }
    
    pLookupTableItem->pPackfileEntry = pEntry;
}

static PACKFILE_ENTRY *VfsFindFileInternalHook(const char *pszFilename)
{
    unsigned Checksum;
    VFS_LOOKUP_TABLE_NEW *pLookupTableItem;
    
    Checksum = VfsCalcFileNameChecksum(pszFilename);
    pLookupTableItem = &g_pVfsLookupTableNew[Checksum % 20713];
    do
    {
        if(pLookupTableItem->pPackfileEntry &&
           pLookupTableItem->pPackfileEntry->dwNameChecksum == Checksum &&
           !stricmp(pLookupTableItem->pPackfileEntry->pszFileName, pszFilename))
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (strstr(pszFilename, DEBUG_VFS_FILENAME1) || strstr(pszFilename, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Found: %s (%x)", pszFilename, Checksum);
#endif
            return pLookupTableItem->pPackfileEntry;
        }
        
        pLookupTableItem = pLookupTableItem->pNext;
    }
    while(pLookupTableItem);
    
    VFS_DBGPRINT("Cannot find: %s (%x)", pszFilename, Checksum);
    
    /*pLookupTableItem = &g_pVfsLookupTableNew[Checksum % 20713];
    do
    {
        if(pLookupTableItem->pPackfileEntry)
        {
            MessageBox(0, pLookupTableItem->pPackfileEntry->pszFileName, "List", 0);
        }
        
        pLookupTableItem = pLookupTableItem->pNext;
    }
    while(pLookupTableItem);*/
    
    return NULL;
}

static void VfsInitHook(void)
{
    memset(g_pVfsLookupTableNew, 0, sizeof(VFS_LOOKUP_TABLE_NEW) * VFS_LOOKUP_TABLE_SIZE);
    if (GetInstalledGameLang() == LANG_GR)
    {
        VfsLoadPackfile("audiog.vpp", 0);
        VfsLoadPackfile("maps_gr.vpp", 0);
        VfsLoadPackfile("levels1g.vpp", 0);
        VfsLoadPackfile("levels2g.vpp", 0);
        VfsLoadPackfile("levels3g.vpp", 0);
        VfsLoadPackfile("ltables.vpp", 0);
    }
    else if (GetInstalledGameLang() == LANG_FR)
    {
        VfsLoadPackfile("audiof.vpp", 0);
        VfsLoadPackfile("maps_fr.vpp", 0);
        VfsLoadPackfile("levels1f.vpp", 0);
        VfsLoadPackfile("levels2f.vpp", 0);
        VfsLoadPackfile("levels3f.vpp", 0);
        VfsLoadPackfile("ltables.vpp", 0);
    }
    else
    {
        VfsLoadPackfile("audio.vpp", 0);
        VfsLoadPackfile("maps_en.vpp", 0);
        VfsLoadPackfile("levels1.vpp", 0);
        VfsLoadPackfile("levels2.vpp", 0);
        VfsLoadPackfile("levels3.vpp", 0);
    }
    VfsLoadPackfile("levelsm.vpp", 0);
    //VfsLoadPackfile("levelseb.vpp", 0);
    //VfsLoadPackfile("levelsbg.vpp", 0);
    VfsLoadPackfile("maps1.vpp", 0);
    VfsLoadPackfile("maps2.vpp", 0);
    VfsLoadPackfile("maps3.vpp", 0);
    VfsLoadPackfile("maps4.vpp", 0);
    VfsLoadPackfile("meshes.vpp", 0);
    VfsLoadPackfile("motions.vpp", 0);
    VfsLoadPackfile("music.vpp", 0);
    VfsLoadPackfile("ui.vpp", 0);
    VfsLoadPackfile("tables.vpp", 0);
    *((BOOL*)0x01BDB218) = TRUE; // bPackfilesLoaded
    *((uint32_t*)0x01BDB210) = 10000; // cFilesInVfs
    *((uint32_t*)0x01BDB214) = 100; // cPackfiles
}

static void VfsCleanupHook(void)
{
    unsigned i;
    for(i = 0; i < g_cPackfiles; ++i)
        free(g_pPackfiles[i]);
    free(g_pPackfiles);
    g_pPackfiles = NULL;
    g_cPackfiles = 0;
}

void VfsApplyHooks(void)
{
    WriteMemUInt8((PVOID)0x0052BCA0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052BCA1, ((ULONG_PTR)VfsAddFileToLookupTableHook) - (0x0052BCA0 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052BD40, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052BD41, ((ULONG_PTR)VfsAddPackfileEntriesHook) - (0x0052BD40 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052C4D0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052C4D1, ((ULONG_PTR)VfsBuildPackfileEntriesListHook) - (0x0052C4D0 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052C070, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052C071, ((ULONG_PTR)VfsLoadPackfileHook) - (0x0052C070 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052C1D0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052C1D1, ((ULONG_PTR)VfsFindPackfileHook) - (0x0052C1D0 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052C220, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052C221, ((ULONG_PTR)VfsFindFileInternalHook) - (0x0052C220 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052BB60, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052BB61, ((ULONG_PTR)VfsInitHook) - (0x0052BB60 + 0x5));
    
    WriteMemUInt8((PVOID)0x0052BC80, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x0052BC81, ((ULONG_PTR)VfsCleanupHook) - (0x0052BC80 + 0x5));

    if (GetInstalledGameLang() != LANG_EN)
    {
        WriteMemPtr((PVOID)(0x0043DCAB + 1), (PVOID)"localized_credits.tbl");
        WriteMemPtr((PVOID)(0x0043E50B + 1), (PVOID)"localized_endgame.tbl");
        WriteMemPtr((PVOID)(0x004B082B + 1), (PVOID)"localized_strings.tbl");
    }

#ifdef DEBUG
    WriteMemUInt8((PVOID)0x0052BEF0, 0xFF); // VfsInitPackfileFilesList
    WriteMemUInt8((PVOID)0x0052BF50, 0xFF); // VfsLoadPackfileInternal
    WriteMemUInt8((PVOID)0x0052C440, 0xFF); // VfsFindPackfileEntry
#endif

    /* Load ui.vpp before tables.vpp - not used anymore */
    //WriteMemPtr((PVOID)0x0052BC58, "ui.vpp");
    //WriteMemPtr((PVOID)0x0052BC67, "tables.vpp");
}

void ForceFileFromPackfile(const char *pszName, const char *pszPackfile)
{
    PACKFILE *pPackfile;
    unsigned i;
    
    pPackfile = VfsFindPackfile(pszPackfile);
    if(pPackfile)
    {
        for(i = 0; i < pPackfile->cFiles; ++i)
        {
            if(!stricmp(pPackfile->pFileList[i].pszFileName, pszName))
                VfsAddFileToLookupTable(&pPackfile->pFileList[i]);
        }
    }
}
