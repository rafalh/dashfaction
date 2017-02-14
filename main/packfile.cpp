#include "stdafx.h"
#include "packfile.h"
#include "utils.h"
#include "rf.h"
#include "main.h"

using namespace rf;

struct VFS_LOOKUP_TABLE_NEW
{
    VFS_LOOKUP_TABLE_NEW *pNext;
    PACKFILE_ENTRY *pPackfileEntry;
};

static unsigned g_cPackfiles = 0, g_cFilesInVfs = 0, g_cNameCollisions = 0;
static PACKFILE **g_pPackfiles = NULL;

constexpr auto g_pVfsLookupTableNew = (VFS_LOOKUP_TABLE_NEW*)0x01BB2AC8; // g_pVfsLookupTable
constexpr auto LOOKUP_TABLE_SIZE = 20713;

#define DEBUG_VFS 0
#define DEBUG_VFS_FILENAME1 "DM RTS MiniGolf 2.1.rfl"
#define DEBUG_VFS_FILENAME2 "JumpPad.wav"

#define VFS_DBGPRINT TRACE

static EGameLang DetectInstalledGameLang()
{
    char szFullPath[MAX_PATH];
    const char *LangCodes[] = { "en", "gr", "fr" };
    for (unsigned i = 0; i < COUNTOF(LangCodes); ++i)
    {
        sprintf(szFullPath, "%smaps_%s.vpp", g_pszRootPath, LangCodes[i]);
        BOOL bExists = PathFileExistsA(szFullPath);
        INFO("Checking file %s: %s", szFullPath, bExists ? "found" : "not found");
        if (bExists)
        {
            INFO("Detected game language: %s", LangCodes[i]);
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
    {
        InstalledGameLang = DetectInstalledGameLang();
        Initialized = true;
    }
    return InstalledGameLang;
}

static BOOL VfsLoadPackfileHook(const char *pszFilename, const char *pszDir)
{
    char szFullPath[256], Buf[0x800];
    BOOL bRet = FALSE;
    unsigned cFilesInBlock, cAdded, OffsetInBlocks;
    
    VFS_DBGPRINT("Load packfile %s %s", pszDir, pszFilename);
    
    if (pszDir && pszDir[0] && pszDir[1] == ':')
        sprintf(szFullPath, "%s%s", pszDir, pszFilename); // absolute path
    else
        sprintf(szFullPath, "%s%s%s", g_pszRootPath, pszDir ? pszDir : "", pszFilename);

    if (!pszFilename || strlen(pszFilename) > 0x1F || strlen(szFullPath) > 0x7F)
    {
        ERR("Packfile name or path too long: %s", szFullPath);
        return FALSE;
    }
    
    for (unsigned i = 0; i < g_cPackfiles; ++i)
        if(!stricmp(g_pPackfiles[i]->szPath, szFullPath))
            return TRUE;

    FILE *pFile = fopen(szFullPath, "rb");
    if (!pFile)
    {
        ERR("Failed to open packfile %s", szFullPath);
        return FALSE;
    }
    
    if (g_cPackfiles % 64 == 0)
    {
        g_pPackfiles = (PACKFILE**)realloc(g_pPackfiles, (g_cPackfiles + 64) * sizeof(PACKFILE*));
        memset(&g_pPackfiles[g_cPackfiles], 0, 64 * sizeof(PACKFILE*));
    }
    
    PACKFILE *pPackfile = (PACKFILE*)malloc(sizeof(*pPackfile));
    if (!pPackfile)
    {
        ERR("malloc failed");
        return FALSE;
    }
    
    strncpy(pPackfile->szName, pszFilename, sizeof(pPackfile->szName) - 1);
    pPackfile->szName[sizeof(pPackfile->szName) - 1] = '\0';
    strncpy(pPackfile->szPath, szFullPath, sizeof(pPackfile->szPath) - 1);
    pPackfile->szPath[sizeof(pPackfile->szPath) - 1] = '\0';
    pPackfile->field_A0 = 0;
    pPackfile->cFiles = 0;
    pPackfile->pFileList = NULL;
    
    // Note: VfsProcessPackfileHeader returns number of files in packfile - result 0 is not always a true error
    if (fread(Buf, sizeof(Buf), 1, pFile) == 1 && VfsProcessPackfileHeader(pPackfile, Buf))
    {
        pPackfile->pFileList = (PACKFILE_ENTRY*)malloc(pPackfile->cFiles * sizeof(PACKFILE_ENTRY));
        memset(pPackfile->pFileList, 0, pPackfile->cFiles * sizeof(PACKFILE_ENTRY));
        cAdded = 0;
        OffsetInBlocks = 1;
        bRet = TRUE;
        
        for (unsigned i = 0; i < pPackfile->cFiles; i += 32)
        {
            if (fread(Buf, sizeof(Buf), 1, pFile) != 1)
            {
                bRet = FALSE;
                ERR("Failed to fread vpp %s", szFullPath);
                break;
            }
            
            cFilesInBlock = std::min(pPackfile->cFiles - i, 32u);
            VfsAddPackfileEntries(pPackfile, Buf, cFilesInBlock, &cAdded);
            ++OffsetInBlocks;
        }
    } else ERR("Failed to fread vpp 2 %s", szFullPath);
    
    if (bRet)
        VfsSetupFileOffsets(pPackfile, OffsetInBlocks);
    
    fclose(pFile);
    
    if (bRet)
    {
        g_pPackfiles[g_cPackfiles] = pPackfile;
        ++g_cPackfiles;
    }
    else
    {
        free(pPackfile->pFileList);
        free(pPackfile);
    }
    
    return bRet;
}

static PACKFILE *VfsFindPackfileHook(const char *pszFilename)
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        if (!stricmp(g_pPackfiles[i]->szName, pszFilename))
            return g_pPackfiles[i];
    }
    
    ERR("Packfile %s not found", pszFilename);
    return NULL;
}

static BOOL VfsBuildPackfileEntriesListHook(const char *pszExtList, char **ppFilenames, unsigned *pcFiles, const char *pszPackfileName)
{
    unsigned cbBuf = 1;
    
    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    *pcFiles = 0;
    *ppFilenames = 0;
    
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        PACKFILE *pPackfile = g_pPackfiles[i];
        if (!pszPackfileName || !stricmp(pszPackfileName, pPackfile->szName))
        {
            for (unsigned j = 0; j < pPackfile->cFiles; ++j)
            {
                const char *pszExt = GetFileExt(pPackfile->pFileList[j].pszFileName);
                if (pszExt[0] && strstr(pszExtList, pszExt + 1))
                    cbBuf += strlen(pPackfile->pFileList[j].pszFileName) + 1;
            }
        }
    }
    
    *ppFilenames = (char*)RfMalloc(cbBuf);
    if (!*ppFilenames)
        return FALSE;
    char *BufPtr = *ppFilenames;
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        PACKFILE *pPackfile = g_pPackfiles[i];
        if (!pszPackfileName || !stricmp(pszPackfileName, pPackfile->szName))
        {
            for (unsigned j = 0; j < pPackfile->cFiles; ++j)
            {
                const char *pszExt = GetFileExt(pPackfile->pFileList[j].pszFileName);
                if (pszExt[0] && strstr(pszExtList, pszExt + 1))
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
    const BYTE *pData = (const BYTE*)pBlock;
    
    for (unsigned i = 0; i < cFilesInBlock; ++i)
    {
        PACKFILE_ENTRY *pEntry = &pPackfile->pFileList[*pcAddedFiles];
        if (*g_pbVfsIgnoreTblFiles && !stricmp(GetFileExt((char*)pData), ".tbl"))
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
     VFS_LOOKUP_TABLE_NEW *pLookupTableItem = &g_pVfsLookupTableNew[pEntry->dwNameChecksum % LOOKUP_TABLE_SIZE];
    
    while (true)
    {
        if (!pLookupTableItem->pPackfileEntry)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME1) || !stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 1: %s (%x)", pEntry->pszFileName, pEntry->dwNameChecksum);
#endif
            break;
        }

        if (!stricmp(pLookupTableItem->pPackfileEntry->pszFileName, pEntry->pszFileName))
        {
            // file with the same name already exist
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME1) || !stricmp(pEntry->pszFileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 2: %s (%x)", pEntry->pszFileName, pEntry->dwNameChecksum);
#endif
            
            const char *pszOldArchive = pLookupTableItem->pPackfileEntry->pArchive->szName;
            const char *pszNewArchive = pEntry->pArchive->szName;

            if (*g_pbVfsIgnoreTblFiles) // this is set to true for user_maps
            {
                if (!g_gameConfig.allowOverwriteGameFiles)
                {
                    TRACE("Denied overwriting game file %s (old packfile %s, new packfile %s)", 
                        pEntry->pszFileName, pszOldArchive, pszNewArchive);
                    return;
                }
                else
                {
                    TRACE("Allowed overwriting game file %s (old packfile %s, new packfile %s)",
                        pEntry->pszFileName, pszOldArchive, pszNewArchive);
                }
            }
            else
            {
                TRACE("Overwriting packfile item %s (old packfile %s, new packfile %s)",
                    pEntry->pszFileName, pszOldArchive, pszNewArchive);
            }
            break;
        }
        
        if (!pLookupTableItem->pNext)
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
        ++g_cNameCollisions;
    }
    
    pLookupTableItem->pPackfileEntry = pEntry;
}

static PACKFILE_ENTRY *VfsFindFileInternalHook(const char *pszFilename)
{
    unsigned Checksum = VfsCalcFileNameChecksum(pszFilename);
    VFS_LOOKUP_TABLE_NEW *pLookupTableItem = &g_pVfsLookupTableNew[Checksum % LOOKUP_TABLE_SIZE];
    do
    {
        if (pLookupTableItem->pPackfileEntry &&
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
    
    /*pLookupTableItem = &g_pVfsLookupTableNew[Checksum % LOOKUP_TABLE_SIZE];
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

    // Load DashFaction specific packfile
    char szBuf[MAX_PATH];
    GetModuleFileNameA(g_hModule, szBuf, sizeof(szBuf));
    char *Ptr = strrchr(szBuf, '\\');
    strcpy(Ptr + 1, "dashfaction.vpp");
    if (PathFileExistsA(szBuf))
        *(Ptr + 1) = '\0';
    else
    {
        // try to remove 2 path components (bin/(debug|release))
        *Ptr = 0;
        Ptr = strrchr(szBuf, '\\');
        if (Ptr)
            *Ptr = 0;
        Ptr = strrchr(szBuf, '\\');
        if (Ptr)
            *(Ptr + 1) = '\0';
    }
    INFO("Loading dashfaction.vpp from directory: %s", szBuf);
    if (!VfsLoadPackfile("dashfaction.vpp", szBuf))
        ERR("Failed to load dashfaction.vpp");

    if (GetInstalledGameLang() == LANG_GR)
    {
        VfsLoadPackfile("audiog.vpp", nullptr);
        VfsLoadPackfile("maps_gr.vpp", nullptr);
        VfsLoadPackfile("levels1g.vpp", nullptr);
        VfsLoadPackfile("levels2g.vpp", nullptr);
        VfsLoadPackfile("levels3g.vpp", nullptr);
        VfsLoadPackfile("ltables.vpp", nullptr);
    }
    else if (GetInstalledGameLang() == LANG_FR)
    {
        VfsLoadPackfile("audiof.vpp", nullptr);
        VfsLoadPackfile("maps_fr.vpp", nullptr);
        VfsLoadPackfile("levels1f.vpp", nullptr);
        VfsLoadPackfile("levels2f.vpp", nullptr);
        VfsLoadPackfile("levels3f.vpp", nullptr);
        VfsLoadPackfile("ltables.vpp", nullptr);
    }
    else
    {
        VfsLoadPackfile("audio.vpp", nullptr);
        VfsLoadPackfile("maps_en.vpp", nullptr);
        VfsLoadPackfile("levels1.vpp", nullptr);
        VfsLoadPackfile("levels2.vpp", nullptr);
        VfsLoadPackfile("levels3.vpp", nullptr);
    }
    VfsLoadPackfile("levelsm.vpp", nullptr);
    //VfsLoadPackfile("levelseb.vpp", nullptr);
    //VfsLoadPackfile("levelsbg.vpp", nullptr);
    VfsLoadPackfile("maps1.vpp", nullptr);
    VfsLoadPackfile("maps2.vpp", nullptr);
    VfsLoadPackfile("maps3.vpp", nullptr);
    VfsLoadPackfile("maps4.vpp", nullptr);
    VfsLoadPackfile("meshes.vpp", nullptr);
    VfsLoadPackfile("motions.vpp", nullptr);
    VfsLoadPackfile("music.vpp", nullptr);
    VfsLoadPackfile("ui.vpp", nullptr);
    VfsLoadPackfile("tables.vpp", nullptr);
    *((BOOL*)0x01BDB218) = TRUE; // bPackfilesLoaded
    *((uint32_t*)0x01BDB210) = 10000; // cFilesInVfs
    *((uint32_t*)0x01BDB214) = 100; // cPackfiles

    // Note: language changes in binary are done here to make sure RootPath is already initialized

    // Switch UI language - can be anything even if this is US edition
    WriteMemUInt8(0x004B27D2 + 1, (uint8_t)GetInstalledGameLang());

    // Switch localized tables names
    if (GetInstalledGameLang() != LANG_EN)
    {
        WriteMemPtr(0x0043DCAB + 1, (PVOID)"localized_credits.tbl");
        WriteMemPtr(0x0043E50B + 1, (PVOID)"localized_endgame.tbl");
        WriteMemPtr(0x004B082B + 1, (PVOID)"localized_strings.tbl");
    }

    INFO("Packfile name collisions: %d", g_cNameCollisions);
}

static void VfsCleanupHook(void)
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
        free(g_pPackfiles[i]);
    free(g_pPackfiles);
    g_pPackfiles = NULL;
    g_cPackfiles = 0;
}

void VfsApplyHooks(void)
{
    WriteMemUInt8(0x0052BCA0, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052BCA0 + 1, (uintptr_t)VfsAddFileToLookupTableHook - (0x0052BCA0 + 0x5));
    
    WriteMemUInt8(0x0052BD40, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052BD40 + 1, (uintptr_t)VfsAddPackfileEntriesHook - (0x0052BD40 + 0x5));
    
    WriteMemUInt8(0x0052C4D0, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052C4D0 + 1, (uintptr_t)VfsBuildPackfileEntriesListHook - (0x0052C4D0 + 0x5));
    
    WriteMemUInt8(0x0052C070, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052C070 + 1, (uintptr_t)VfsLoadPackfileHook - (0x0052C070 + 0x5));
    
    WriteMemUInt8(0x0052C1D0, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052C1D0 + 1, (uintptr_t)VfsFindPackfileHook - (0x0052C1D0 + 0x5));
    
    WriteMemUInt8(0x0052C220, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052C220 + 1, (uintptr_t)VfsFindFileInternalHook - (0x0052C220 + 0x5));
    
    WriteMemUInt8(0x0052BB60, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052BB60 + 1, (uintptr_t)VfsInitHook - (0x0052BB60 + 0x5));
    
    WriteMemUInt8(0x0052BC80, ASM_LONG_JMP_REL);
    WriteMemInt32(0x0052BC80 + 1, (uintptr_t)VfsCleanupHook - (0x0052BC80 + 0x5));

#ifdef DEBUG
    WriteMemUInt8(0x0052BEF0, 0xFF); // VfsInitPackfileFilesList
    WriteMemUInt8(0x0052BF50, 0xFF); // VfsLoadPackfileInternal
    WriteMemUInt8(0x0052C440, 0xFF); // VfsFindPackfileEntry
#endif

    /* Load ui.vpp before tables.vpp - not used anymore */
    //WriteMemPtr(0x0052BC58, "ui.vpp");
    //WriteMemPtr(0x0052BC67, "tables.vpp");
}

void ForceFileFromPackfile(const char *pszName, const char *pszPackfile)
{
    PACKFILE *pPackfile = VfsFindPackfile(pszPackfile);
    if (pPackfile)
    {
        for (unsigned i = 0; i < pPackfile->cFiles; ++i)
        {
            if (!stricmp(pPackfile->pFileList[i].pszFileName, pszName))
                VfsAddFileToLookupTable(&pPackfile->pFileList[i]);
        }
    }
}
