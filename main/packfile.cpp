#include "stdafx.h"
#include "packfile.h"
#include "utils.h"
#include "rf.h"
#include "main.h"
#include "xxhash.h"

#define DEBUG_VFS 0
#define DEBUG_VFS_FILENAME1 "DM RTS MiniGolf 2.1.rfl"
#define DEBUG_VFS_FILENAME2 "JumpPad.wav"

#define VFS_DBGPRINT TRACE

namespace rf {
struct PackfileEntry;

struct Packfile
{
    char szName[32];
    char szPath[128];
    uint32_t field_A0;
    uint32_t cFiles;
    PackfileEntry *pFileList;
    uint32_t cbSize;
};

struct PackfileEntry
{
    uint32_t dwNameChecksum;
    const char *pszFileName;
    uint32_t OffsetInBlocks;
    uint32_t cbFileSize;
    Packfile *pArchive;
    FILE *pRawFile;
};

struct PackfileLookupTable
{
    uint32_t dwNameChecksum;
    PackfileEntry *pArchiveEntry;
};

static auto &g_cArchives = *(uint32_t*)0x01BDB214;
static auto &g_Archives = *(Packfile*)0x01BA7AC8;
#define VFS_LOOKUP_TABLE_SIZE 20713
static auto &g_VfsLookupTable = *(PackfileLookupTable*)0x01BB2AC8;
static auto &g_bVfsIgnoreTblFiles = *(uint8_t*)0x01BDB21C;

typedef Packfile *(*PackfileFindArchive_Type)(const char *pszFilename);
static const auto PackfileFindArchive = (PackfileFindArchive_Type)0x0052C1D0;

typedef uint32_t(*PackfileCalcFileNameChecksum_Type)(const char *pszFileName);
static const auto PackfileCalcFileNameChecksum = (PackfileCalcFileNameChecksum_Type)0x0052BE70;

typedef uint32_t(*PackfileAddToLookupTable_Type)(PackfileEntry *pPackfileEntry);
static const auto PackfileAddToLookupTable = (PackfileAddToLookupTable_Type)0x0052BCA0;

typedef uint32_t(*PackfileProcessHeader_Type)(Packfile *pPackfile, const void *pHeader);
static const auto PackfileProcessHeader = (PackfileProcessHeader_Type)0x0052BD10;

typedef uint32_t(*PackfileAddEntries_Type)(Packfile *pPackfile, const void *pBuf, unsigned cFilesInBlock, unsigned *pcAddedEntries);
static const auto PackfileAddEntries = (PackfileAddEntries_Type)0x0052BD40;

typedef uint32_t(*PackfileSetupFileOffsets_Type)(Packfile *pPackfile, unsigned DataOffsetInBlocks);
static const auto PackfileSetupFileOffsets = (PackfileSetupFileOffsets_Type)0x0052BEB0;

static const auto FileGetChecksum = (unsigned(*)(const char *pszFilename))0x00436630;
}

struct PackfileLookupTableNew
{
    PackfileLookupTableNew *pNext;
    rf::PackfileEntry *pPackfileEntry;
};

const char *ModFileWhitelist[] = {
    "reticle_0.tga",
    "scope_ret_0.tga",
    "reticle_rocket_0.tga",
};

#if CHECK_PACKFILE_CHECKSUM

const std::map<std::string, unsigned> GameFileChecksums = {
    // Note: Multiplayer level checksum is checked when loading level
    //{ "levelsm.vpp", 0x17D0D38A },
    // Note: maps are big and slow downs startup
    { "maps1.vpp", 0x52EE4F99 },
    { "maps2.vpp", 0xB053486F },
    { "maps3.vpp", 0xA5ED6271 },
    { "maps4.vpp", 0xE0AB4397 },
    { "meshes.vpp", 0xEBA19172 },
    { "motions.vpp", 0x17132D8E },
    { "tables.vpp", 0x549DAABF },
};

#endif // CHECK_PACKFILE_CHECKSUM

static const auto g_pVfsLookupTableNew = (PackfileLookupTableNew*)0x01BB2AC8; // g_pVfsLookupTable
static const auto LOOKUP_TABLE_SIZE = 20713;

static unsigned g_cPackfiles = 0, g_cFilesInVfs = 0, g_cNameCollisions = 0;
static rf::Packfile **g_pPackfiles = nullptr;
static bool g_bModdedGame = false;

static bool IsModFileInWhitelist(const char *Filename)
{
    for (int i = 0; i < COUNTOF(ModFileWhitelist); ++i)
        if (!stricmp(ModFileWhitelist[i], Filename))
            return true;
}

static unsigned HashFile(const char *Filename)
{
    FILE *pFile = fopen(Filename, "rb");
    if (!pFile) return 0;

    XXH32_state_t *state = XXH32_createState();
    XXH32_reset(state, 0);

    char Buffer[4096];
    while (true)
    {
        size_t len = fread(Buffer, 1, sizeof(Buffer), pFile);
        if (!len) break;
        XXH32_update(state, Buffer, len);
    }
    fclose(pFile);
    XXH32_hash_t hash = XXH32_digest(state);
    XXH32_freeState(state);
    return hash;
}

static GameLang DetectInstalledGameLang()
{
    char szFullPath[MAX_PATH];
    const char *LangCodes[] = { "en", "gr", "fr" };
    for (unsigned i = 0; i < COUNTOF(LangCodes); ++i)
    {
        sprintf(szFullPath, "%smaps_%s.vpp", rf::g_pszRootPath, LangCodes[i]);
        BOOL bExists = PathFileExistsA(szFullPath);
        INFO("Checking file %s: %s", szFullPath, bExists ? "found" : "not found");
        if (bExists)
        {
            INFO("Detected game language: %s", LangCodes[i]);
            return (GameLang)i;
        }
    }
    WARN("Cannot detect game language");
    return LANG_EN; // default
}

GameLang GetInstalledGameLang()
{
    static bool Initialized = false;
    static GameLang InstalledGameLang;
    if (!Initialized)
    {
        InstalledGameLang = DetectInstalledGameLang();
        Initialized = true;
    }
    return InstalledGameLang;
}

bool IsModdedGame()
{
    return g_bModdedGame;
}

static int PackfileLoad_New(const char *pszFilename, const char *pszDir)
{
    char szFullPath[256], Buf[0x800];
    int bRet = 0;
    unsigned cFilesInBlock, cAdded, OffsetInBlocks;
    
    VFS_DBGPRINT("Load packfile %s %s", pszDir, pszFilename);
    
    if (pszDir && pszDir[0] && pszDir[1] == ':')
        sprintf(szFullPath, "%s%s", pszDir, pszFilename); // absolute path
    else
        sprintf(szFullPath, "%s%s%s", rf::g_pszRootPath, pszDir ? pszDir : "", pszFilename);

    if (!pszFilename || strlen(pszFilename) > 0x1F || strlen(szFullPath) > 0x7F)
    {
        ERR("Packfile name or path too long: %s", szFullPath);
        return 0;
    }
    
    for (unsigned i = 0; i < g_cPackfiles; ++i)
        if (!stricmp(g_pPackfiles[i]->szPath, szFullPath))
            return 1;

#if CHECK_PACKFILE_CHECKSUM
    if (!pszDir)
    {
        auto it = GameFileChecksums.find(pszFilename);
        if (it != GameFileChecksums.end())
        {
            unsigned Checksum = HashFile(szFullPath);
            if (Checksum != it->second)
            {
                INFO("Packfile %s has invalid checksum 0x%X", pszFilename, Checksum);
                g_bModdedGame = true;
            }
        }
    }
#endif // CHECK_PACKFILE_CHECKSUM

    FILE *pFile = fopen(szFullPath, "rb");
    if (!pFile)
    {
        ERR("Failed to open packfile %s", szFullPath);
        return 0;
    }
    
    if (g_cPackfiles % 64 == 0)
    {
        g_pPackfiles = (rf::Packfile**)realloc(g_pPackfiles, (g_cPackfiles + 64) * sizeof(rf::Packfile*));
        memset(&g_pPackfiles[g_cPackfiles], 0, 64 * sizeof(rf::Packfile*));
    }
    
    rf::Packfile *pPackfile = (rf::Packfile*)malloc(sizeof(*pPackfile));
    if (!pPackfile)
    {
        ERR("malloc failed");
        return 0;
    }
    
    strncpy(pPackfile->szName, pszFilename, sizeof(pPackfile->szName) - 1);
    pPackfile->szName[sizeof(pPackfile->szName) - 1] = '\0';
    strncpy(pPackfile->szPath, szFullPath, sizeof(pPackfile->szPath) - 1);
    pPackfile->szPath[sizeof(pPackfile->szPath) - 1] = '\0';
    pPackfile->field_A0 = 0;
    pPackfile->cFiles = 0;
    pPackfile->pFileList = NULL;
    
    // Note: VfsProcessPackfileHeader returns number of files in packfile - result 0 is not always a true error
    if (fread(Buf, sizeof(Buf), 1, pFile) == 1 && rf::PackfileProcessHeader(pPackfile, Buf))
    {
        pPackfile->pFileList = (rf::PackfileEntry*)malloc(pPackfile->cFiles * sizeof(rf::PackfileEntry));
        memset(pPackfile->pFileList, 0, pPackfile->cFiles * sizeof(rf::PackfileEntry));
        cAdded = 0;
        OffsetInBlocks = 1;
        bRet = 1;
        
        for (unsigned i = 0; i < pPackfile->cFiles; i += 32)
        {
            if (fread(Buf, sizeof(Buf), 1, pFile) != 1)
            {
                bRet = 0;
                ERR("Failed to fread vpp %s", szFullPath);
                break;
            }
            
            cFilesInBlock = std::min(pPackfile->cFiles - i, 32u);
            rf::PackfileAddEntries(pPackfile, Buf, cFilesInBlock, &cAdded);
            ++OffsetInBlocks;
        }
    } else ERR("Failed to fread vpp 2 %s", szFullPath);
    
    if (bRet)
        rf::PackfileSetupFileOffsets(pPackfile, OffsetInBlocks);
    
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

static rf::Packfile *PackfileFindArchive_New(const char *pszFilename)
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        if (!stricmp(g_pPackfiles[i]->szName, pszFilename))
            return g_pPackfiles[i];
    }
    
    ERR("Packfile %s not found", pszFilename);
    return NULL;
}

static int PackfileBuildEntriesList_New(const char *pszExtList, char **ppFilenames, unsigned *pcFiles, const char *pszPackfileName)
{
    unsigned cbBuf = 1;
    
    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    *pcFiles = 0;
    *ppFilenames = 0;
    
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        rf::Packfile *pPackfile = g_pPackfiles[i];
        if (!pszPackfileName || !stricmp(pszPackfileName, pPackfile->szName))
        {
            for (unsigned j = 0; j < pPackfile->cFiles; ++j)
            {
                const char *pszExt = rf::GetFileExt(pPackfile->pFileList[j].pszFileName);
                if (pszExt[0] && strstr(pszExtList, pszExt + 1))
                    cbBuf += strlen(pPackfile->pFileList[j].pszFileName) + 1;
            }
        }
    }
    
    *ppFilenames = (char*)rf::Malloc(cbBuf);
    if (!*ppFilenames)
        return 0;
    char *BufPtr = *ppFilenames;
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        rf::Packfile *pPackfile = g_pPackfiles[i];
        if (!pszPackfileName || !stricmp(pszPackfileName, pPackfile->szName))
        {
            for (unsigned j = 0; j < pPackfile->cFiles; ++j)
            {
                const char *pszExt = rf::GetFileExt(pPackfile->pFileList[j].pszFileName);
                if (pszExt[0] && strstr(pszExtList, pszExt + 1))
                {
                    strcpy(BufPtr, pPackfile->pFileList[j].pszFileName);
                    BufPtr += strlen(pPackfile->pFileList[j].pszFileName) + 1;
                    ++(*pcFiles);
                }
            }
            
        }
    }
    // terminating zero
    BufPtr[0] = 0;
    
    return 1;
}

static int PackfileAddEntries_New(rf::Packfile *pPackfile, const void *pBlock, unsigned cFilesInBlock, unsigned *pcAddedFiles)
{
    const uint8_t *pData = (const uint8_t*)pBlock;
    
    for (unsigned i = 0; i < cFilesInBlock; ++i)
    {
        rf::PackfileEntry *pEntry = &pPackfile->pFileList[*pcAddedFiles];
        if (rf::g_bVfsIgnoreTblFiles && !stricmp(rf::GetFileExt((char*)pData), ".tbl"))
            pEntry->pszFileName = "DEADBEEF";
        else
        {
            // FIXME: free?
            char *pFileNameBuf = (char*)malloc(strlen((char*)pData) + 10);
            memset(pFileNameBuf, 0, strlen((char*)pData) + 10);
            strcpy(pFileNameBuf, (char*)pData);
            pEntry->pszFileName = pFileNameBuf;
        }
        pEntry->dwNameChecksum = rf::PackfileCalcFileNameChecksum(pEntry->pszFileName);
        pEntry->cbFileSize = *((uint32_t*)(pData + 60));
        pEntry->pArchive = pPackfile;
        pEntry->pRawFile = NULL;
        
        pData += 64;
        ++(*pcAddedFiles);
        
        rf::PackfileAddToLookupTable(pEntry);
        ++g_cFilesInVfs;
    }
    return 1;
}

static void PackfileAddToLookupTable_New(rf::PackfileEntry *pEntry)
{
    PackfileLookupTableNew *pLookupTableItem = &g_pVfsLookupTableNew[pEntry->dwNameChecksum % LOOKUP_TABLE_SIZE];
    
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

            if (rf::g_bVfsIgnoreTblFiles) // this is set to true for user_maps
            {
                bool bWhitelisted = false;// IsModFileInWhitelist(pEntry->pszFileName);
                if (!g_game_config.allowOverwriteGameFiles && !bWhitelisted)
                {
                    TRACE("Denied overwriting game file %s (old packfile %s, new packfile %s)", 
                        pEntry->pszFileName, pszOldArchive, pszNewArchive);
                    return;
                }
                else
                {
                    TRACE("Allowed overwriting game file %s (old packfile %s, new packfile %s)",
                        pEntry->pszFileName, pszOldArchive, pszNewArchive);
                    if (!bWhitelisted)
                        g_bModdedGame = true;
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
            
            pLookupTableItem->pNext = (PackfileLookupTableNew*)malloc(sizeof(PackfileLookupTableNew));
            pLookupTableItem = pLookupTableItem->pNext;
            pLookupTableItem->pNext = NULL;
            break;
        }

        pLookupTableItem = pLookupTableItem->pNext;
        ++g_cNameCollisions;
    }
    
    pLookupTableItem->pPackfileEntry = pEntry;
}

static rf::PackfileEntry *PackfileFindFileInternal_New(const char *pszFilename)
{
    unsigned Checksum = rf::PackfileCalcFileNameChecksum(pszFilename);
    PackfileLookupTableNew *pLookupTableItem = &g_pVfsLookupTableNew[Checksum % LOOKUP_TABLE_SIZE];
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

static void LoadDashFactionVpp()
{
    // Load DashFaction specific packfile
    char szBuf[MAX_PATH];
    GetModuleFileNameA(g_hmodule, szBuf, sizeof(szBuf));
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
    if (!rf::PackfileLoad("dashfaction.vpp", szBuf))
        ERR("Failed to load dashfaction.vpp");
}

static void PackfileInit_New()
{
    unsigned StartTicks = GetTickCount();

    memset(g_pVfsLookupTableNew, 0, sizeof(PackfileLookupTableNew) * VFS_LOOKUP_TABLE_SIZE);

    if (GetInstalledGameLang() == LANG_GR)
    {
        rf::PackfileLoad("audiog.vpp", nullptr);
        rf::PackfileLoad("maps_gr.vpp", nullptr);
        rf::PackfileLoad("levels1g.vpp", nullptr);
        rf::PackfileLoad("levels2g.vpp", nullptr);
        rf::PackfileLoad("levels3g.vpp", nullptr);
        rf::PackfileLoad("ltables.vpp", nullptr);
    }
    else if (GetInstalledGameLang() == LANG_FR)
    {
        rf::PackfileLoad("audiof.vpp", nullptr);
        rf::PackfileLoad("maps_fr.vpp", nullptr);
        rf::PackfileLoad("levels1f.vpp", nullptr);
        rf::PackfileLoad("levels2f.vpp", nullptr);
        rf::PackfileLoad("levels3f.vpp", nullptr);
        rf::PackfileLoad("ltables.vpp", nullptr);
    }
    else
    {
        rf::PackfileLoad("audio.vpp", nullptr);
        rf::PackfileLoad("maps_en.vpp", nullptr);
        rf::PackfileLoad("levels1.vpp", nullptr);
        rf::PackfileLoad("levels2.vpp", nullptr);
        rf::PackfileLoad("levels3.vpp", nullptr);
    }
    rf::PackfileLoad("levelsm.vpp", nullptr);
    //rf::PackfileLoad("levelseb.vpp", nullptr);
    //rf::PackfileLoad("levelsbg.vpp", nullptr);
    rf::PackfileLoad("maps1.vpp", nullptr);
    rf::PackfileLoad("maps2.vpp", nullptr);
    rf::PackfileLoad("maps3.vpp", nullptr);
    rf::PackfileLoad("maps4.vpp", nullptr);
    rf::PackfileLoad("meshes.vpp", nullptr);
    rf::PackfileLoad("motions.vpp", nullptr);
    rf::PackfileLoad("music.vpp", nullptr);
    rf::PackfileLoad("ui.vpp", nullptr);
    LoadDashFactionVpp();
    rf::PackfileLoad("tables.vpp", nullptr);
    *((int*)0x01BDB218) = 1; // bPackfilesLoaded
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

    INFO("Packfiles initialization took %dms", GetTickCount() - StartTicks);
    INFO("Packfile name collisions: %d", g_cNameCollisions);

    if (g_bModdedGame)
        INFO("Modded game detected!");
}

static void PackfileCleanup_New()
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
        free(g_pPackfiles[i]);
    free(g_pPackfiles);
    g_pPackfiles = nullptr;
    g_cPackfiles = 0;
}

void VfsApplyHooks()
{
    AsmWritter(0x0052BCA0).jmpLong(PackfileAddToLookupTable_New);
    AsmWritter(0x0052BD40).jmpLong(PackfileAddEntries_New);
    AsmWritter(0x0052C4D0).jmpLong(PackfileBuildEntriesList_New);
    AsmWritter(0x0052C070).jmpLong(PackfileLoad_New);
    AsmWritter(0x0052C1D0).jmpLong(PackfileFindArchive_New);
    AsmWritter(0x0052C220).jmpLong(PackfileFindFileInternal_New);
    AsmWritter(0x0052BB60).jmpLong(PackfileInit_New);
    AsmWritter(0x0052BC80).jmpLong(PackfileCleanup_New);

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
    rf::Packfile *pPackfile = rf::PackfileFindArchive(pszPackfile);
    if (pPackfile)
    {
        for (unsigned i = 0; i < pPackfile->cFiles; ++i)
        {
            if (!stricmp(pPackfile->pFileList[i].pszFileName, pszName))
                rf::PackfileAddToLookupTable(&pPackfile->pFileList[i]);
        }
    }
}

void PackfileFindMatchingFiles(const StringMatcher &Query, std::function<void(const char *)> ResultConsumer)
{
    for (int i = 0; i < LOOKUP_TABLE_SIZE; ++i)
    {
        PackfileLookupTableNew *pLookupTableItem = &g_pVfsLookupTableNew[i];
        if (!pLookupTableItem->pPackfileEntry)
            continue;

        do
        {
            const char *pszFilename = pLookupTableItem->pPackfileEntry->pszFileName;
            if (Query(pszFilename))
                ResultConsumer(pLookupTableItem->pPackfileEntry->pszFileName);
            pLookupTableItem = pLookupTableItem->pNext;
        } while (pLookupTableItem);
    }
}
