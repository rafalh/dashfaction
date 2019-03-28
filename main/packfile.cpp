#include "stdafx.h"
#include "packfile.h"
#include "utils.h"
#include "rf.h"
#include "main.h"
#include "xxhash.h"
#include <ShortTypes.h>

#define DEBUG_VFS 0
#define DEBUG_VFS_FILENAME1 "DM RTS MiniGolf 2.1.rfl"
#define DEBUG_VFS_FILENAME2 "JumpPad.wav"

#define VFS_DBGPRINT TRACE

namespace rf {
struct PackfileEntry;

struct Packfile
{
    char Name[32];
    char Path[128];
    uint32_t field_A0;
    uint32_t cFiles;
    PackfileEntry *FileList;
    uint32_t cbSize;
};

struct PackfileEntry
{
    uint32_t dwNameChecksum;
    const char *FileName;
    uint32_t OffsetInBlocks;
    uint32_t cbFileSize;
    Packfile *Archive;
    FILE *RawFile;
};

struct PackfileLookupTable
{
    uint32_t dwNameChecksum;
    PackfileEntry *ArchiveEntry;
};

static auto &g_cArchives = *(uint32_t*)0x01BDB214;
static auto &g_Archives = *(Packfile*)0x01BA7AC8;
#define VFS_LOOKUP_TABLE_SIZE 20713
static auto &g_VfsLookupTable = *(PackfileLookupTable*)0x01BB2AC8;
static auto &g_VfsIgnoreTblFiles = *(uint8_t*)0x01BDB21C;

typedef Packfile *(*PackfileFindArchive_Type)(const char *Filename);
static const auto PackfileFindArchive = (PackfileFindArchive_Type)0x0052C1D0;

typedef uint32_t(*PackfileCalcFileNameChecksum_Type)(const char *FileName);
static const auto PackfileCalcFileNameChecksum = (PackfileCalcFileNameChecksum_Type)0x0052BE70;

typedef uint32_t(*PackfileAddToLookupTable_Type)(PackfileEntry *PackfileEntry);
static const auto PackfileAddToLookupTable = (PackfileAddToLookupTable_Type)0x0052BCA0;

typedef uint32_t(*PackfileProcessHeader_Type)(Packfile *Packfile, const void *Header);
static const auto PackfileProcessHeader = (PackfileProcessHeader_Type)0x0052BD10;

typedef uint32_t(*PackfileAddEntries_Type)(Packfile *Packfile, const void *Buf, unsigned cFilesInBlock, unsigned *pcAddedEntries);
static const auto PackfileAddEntries = (PackfileAddEntries_Type)0x0052BD40;

typedef uint32_t(*PackfileSetupFileOffsets_Type)(Packfile *Packfile, unsigned DataOffsetInBlocks);
static const auto PackfileSetupFileOffsets = (PackfileSetupFileOffsets_Type)0x0052BEB0;

static const auto FileGetChecksum = (unsigned(*)(const char *Filename))0x00436630;
}

struct PackfileLookupTableNew
{
    PackfileLookupTableNew *Next;
    rf::PackfileEntry *PackfileEntry;
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

static const auto g_VfsLookupTableNew = (PackfileLookupTableNew*)0x01BB2AC8; // g_VfsLookupTable
static const auto LOOKUP_TABLE_SIZE = 20713;

static unsigned g_cPackfiles = 0, g_cFilesInVfs = 0, g_cNameCollisions = 0;
static rf::Packfile **g_Packfiles = nullptr;
static bool g_ModdedGame = false;

#ifdef MOD_FILE_WHITELIST

const char *ModFileWhitelist[] = {
    "reticle_0.tga",
    "scope_ret_0.tga",
    "reticle_rocket_0.tga",
};

static bool IsModFileInWhitelist(const char *Filename)
{
    for (unsigned i = 0; i < COUNTOF(ModFileWhitelist); ++i)
        if (!stricmp(ModFileWhitelist[i], Filename))
            return true;
    return false;
}

#endif // MOD_FILE_WHITELIST

#if CHECK_PACKFILE_CHECKSUM

static unsigned HashFile(const char *Filename)
{
    FILE *File = fopen(Filename, "rb");
    if (!File) return 0;

    XXH32_state_t *state = XXH32_createState();
    XXH32_reset(state, 0);

    char Buffer[4096];
    while (true)
    {
        size_t len = fread(Buffer, 1, sizeof(Buffer), File);
        if (!len) break;
        XXH32_update(state, Buffer, len);
    }
    fclose(File);
    XXH32_hash_t hash = XXH32_digest(state);
    XXH32_freeState(state);
    return hash;
}
#endif // CHECK_PACKFILE_CHECKSUM

static GameLang DetectInstalledGameLang()
{
    char FullPath[MAX_PATH];
    const char *LangCodes[] = { "en", "gr", "fr" };
    for (unsigned i = 0; i < COUNTOF(LangCodes); ++i)
    {
        sprintf(FullPath, "%smaps_%s.vpp", rf::g_RootPath, LangCodes[i]);
        BOOL Exists = PathFileExistsA(FullPath);
        INFO("Checking file %s: %s", FullPath, Exists ? "found" : "not found");
        if (Exists)
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
    return g_ModdedGame;
}

static int PackfileLoad_New(const char *Filename, const char *Dir)
{
    char FullPath[256], Buf[0x800];
    int Ret = 0;
    unsigned cFilesInBlock, cAdded, OffsetInBlocks;

    VFS_DBGPRINT("Load packfile %s %s", Dir, Filename);

    if (Dir && Dir[0] && Dir[1] == ':')
        sprintf(FullPath, "%s%s", Dir, Filename); // absolute path
    else
        sprintf(FullPath, "%s%s%s", rf::g_RootPath, Dir ? Dir : "", Filename);

    if (!Filename || strlen(Filename) > 0x1F || strlen(FullPath) > 0x7F)
    {
        ERR("Packfile name or path too long: %s", FullPath);
        return 0;
    }

    for (unsigned i = 0; i < g_cPackfiles; ++i)
        if (!stricmp(g_Packfiles[i]->Path, FullPath))
            return 1;

#if CHECK_PACKFILE_CHECKSUM
    if (!Dir)
    {
        auto it = GameFileChecksums.find(Filename);
        if (it != GameFileChecksums.end())
        {
            unsigned Checksum = HashFile(FullPath);
            if (Checksum != it->second)
            {
                INFO("Packfile %s has invalid checksum 0x%X", Filename, Checksum);
                g_ModdedGame = true;
            }
        }
    }
#endif // CHECK_PACKFILE_CHECKSUM

    FILE *File = fopen(FullPath, "rb");
    if (!File)
    {
        ERR("Failed to open packfile %s", FullPath);
        return 0;
    }

    if (g_cPackfiles % 64 == 0)
    {
        g_Packfiles = (rf::Packfile**)realloc(g_Packfiles, (g_cPackfiles + 64) * sizeof(rf::Packfile*));
        memset(&g_Packfiles[g_cPackfiles], 0, 64 * sizeof(rf::Packfile*));
    }

    rf::Packfile *Packfile = (rf::Packfile*)malloc(sizeof(*Packfile));
    if (!Packfile)
    {
        ERR("malloc failed");
        return 0;
    }

    strncpy(Packfile->Name, Filename, sizeof(Packfile->Name) - 1);
    Packfile->Name[sizeof(Packfile->Name) - 1] = '\0';
    strncpy(Packfile->Path, FullPath, sizeof(Packfile->Path) - 1);
    Packfile->Path[sizeof(Packfile->Path) - 1] = '\0';
    Packfile->field_A0 = 0;
    Packfile->cFiles = 0;
    Packfile->FileList = NULL;

    // Note: VfsProcessPackfileHeader returns number of files in packfile - result 0 is not always a true error
    if (fread(Buf, sizeof(Buf), 1, File) == 1 && rf::PackfileProcessHeader(Packfile, Buf))
    {
        Packfile->FileList = (rf::PackfileEntry*)malloc(Packfile->cFiles * sizeof(rf::PackfileEntry));
        memset(Packfile->FileList, 0, Packfile->cFiles * sizeof(rf::PackfileEntry));
        cAdded = 0;
        OffsetInBlocks = 1;
        Ret = 1;

        for (unsigned i = 0; i < Packfile->cFiles; i += 32)
        {
            if (fread(Buf, sizeof(Buf), 1, File) != 1)
            {
                Ret = 0;
                ERR("Failed to fread vpp %s", FullPath);
                break;
            }

            cFilesInBlock = std::min(Packfile->cFiles - i, 32u);
            rf::PackfileAddEntries(Packfile, Buf, cFilesInBlock, &cAdded);
            ++OffsetInBlocks;
        }
    } else ERR("Failed to fread vpp 2 %s", FullPath);

    if (Ret)
        rf::PackfileSetupFileOffsets(Packfile, OffsetInBlocks);

    fclose(File);

    if (Ret)
    {
        g_Packfiles[g_cPackfiles] = Packfile;
        ++g_cPackfiles;
    }
    else
    {
        free(Packfile->FileList);
        free(Packfile);
    }

    return Ret;
}

static rf::Packfile *PackfileFindArchive_New(const char *Filename)
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        if (!stricmp(g_Packfiles[i]->Name, Filename))
            return g_Packfiles[i];
    }

    ERR("Packfile %s not found", Filename);
    return NULL;
}

static int PackfileBuildEntriesList_New(const char *ExtList, char **ppFilenames, unsigned *pcFiles, const char *PackfileName)
{
    unsigned cbBuf = 1;

    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    *pcFiles = 0;
    *ppFilenames = 0;

    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        rf::Packfile *Packfile = g_Packfiles[i];
        if (!PackfileName || !stricmp(PackfileName, Packfile->Name))
        {
            for (unsigned j = 0; j < Packfile->cFiles; ++j)
            {
                const char *Ext = rf::GetFileExt(Packfile->FileList[j].FileName);
                if (Ext[0] && strstr(ExtList, Ext + 1))
                    cbBuf += strlen(Packfile->FileList[j].FileName) + 1;
            }
        }
    }

    *ppFilenames = (char*)rf::Malloc(cbBuf);
    if (!*ppFilenames)
        return 0;
    char *BufPtr = *ppFilenames;
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        rf::Packfile *Packfile = g_Packfiles[i];
        if (!PackfileName || !stricmp(PackfileName, Packfile->Name))
        {
            for (unsigned j = 0; j < Packfile->cFiles; ++j)
            {
                const char *Ext = rf::GetFileExt(Packfile->FileList[j].FileName);
                if (Ext[0] && strstr(ExtList, Ext + 1))
                {
                    strcpy(BufPtr, Packfile->FileList[j].FileName);
                    BufPtr += strlen(Packfile->FileList[j].FileName) + 1;
                    ++(*pcFiles);
                }
            }

        }
    }
    // terminating zero
    BufPtr[0] = 0;

    return 1;
}

static int PackfileAddEntries_New(rf::Packfile *Packfile, const void *Block, unsigned cFilesInBlock, unsigned *pcAddedFiles)
{
    const uint8_t *Data = (const uint8_t*)Block;

    for (unsigned i = 0; i < cFilesInBlock; ++i)
    {
        rf::PackfileEntry *Entry = &Packfile->FileList[*pcAddedFiles];
        if (rf::g_VfsIgnoreTblFiles && !stricmp(rf::GetFileExt((char*)Data), ".tbl"))
            Entry->FileName = "DEADBEEF";
        else
        {
            // FIXME: free?
            char *FileNameBuf = (char*)malloc(strlen((char*)Data) + 10);
            memset(FileNameBuf, 0, strlen((char*)Data) + 10);
            strcpy(FileNameBuf, (char*)Data);
            Entry->FileName = FileNameBuf;
        }
        Entry->dwNameChecksum = rf::PackfileCalcFileNameChecksum(Entry->FileName);
        Entry->cbFileSize = *((uint32_t*)(Data + 60));
        Entry->Archive = Packfile;
        Entry->RawFile = NULL;

        Data += 64;
        ++(*pcAddedFiles);

        rf::PackfileAddToLookupTable(Entry);
        ++g_cFilesInVfs;
    }
    return 1;
}

static void PackfileAddToLookupTable_New(rf::PackfileEntry *Entry)
{
    PackfileLookupTableNew *LookupTableItem = &g_VfsLookupTableNew[Entry->dwNameChecksum % LOOKUP_TABLE_SIZE];

    while (true)
    {
        if (!LookupTableItem->PackfileEntry)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 1: %s (%x)", Entry->FileName, Entry->dwNameChecksum);
#endif
            break;
        }

        if (!stricmp(LookupTableItem->PackfileEntry->FileName, Entry->FileName))
        {
            // file with the same name already exist
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 2: %s (%x)", Entry->FileName, Entry->dwNameChecksum);
#endif

            const char *OldArchive = LookupTableItem->PackfileEntry->Archive->Name;
            const char *NewArchive = Entry->Archive->Name;

            if (rf::g_VfsIgnoreTblFiles) // this is set to true for user_maps
            {
                bool Whitelisted = false;
#ifdef MOD_FILE_WHITELIST
                Whitelisted = IsModFileInWhitelist(Entry->FileName);
#endif
                if (!g_game_config.allowOverwriteGameFiles && !Whitelisted)
                {
                    TRACE("Denied overwriting game file %s (old packfile %s, new packfile %s)",
                        Entry->FileName, OldArchive, NewArchive);
                    return;
                }
                else
                {
                    TRACE("Allowed overwriting game file %s (old packfile %s, new packfile %s)",
                        Entry->FileName, OldArchive, NewArchive);
                    if (!Whitelisted)
                        g_ModdedGame = true;
                }
            }
            else
            {
                TRACE("Overwriting packfile item %s (old packfile %s, new packfile %s)",
                    Entry->FileName, OldArchive, NewArchive);
            }
            break;
        }

        if (!LookupTableItem->Next)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 3: %s (%x)", Entry->FileName, Entry->dwNameChecksum);
#endif

            LookupTableItem->Next = (PackfileLookupTableNew*)malloc(sizeof(PackfileLookupTableNew));
            LookupTableItem = LookupTableItem->Next;
            LookupTableItem->Next = NULL;
            break;
        }

        LookupTableItem = LookupTableItem->Next;
        ++g_cNameCollisions;
    }

    LookupTableItem->PackfileEntry = Entry;
}

static rf::PackfileEntry *PackfileFindFileInternal_New(const char *Filename)
{
    unsigned Checksum = rf::PackfileCalcFileNameChecksum(Filename);
    PackfileLookupTableNew *LookupTableItem = &g_VfsLookupTableNew[Checksum % LOOKUP_TABLE_SIZE];
    do
    {
        if (LookupTableItem->PackfileEntry &&
           LookupTableItem->PackfileEntry->dwNameChecksum == Checksum &&
           !stricmp(LookupTableItem->PackfileEntry->FileName, Filename))
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (strstr(Filename, DEBUG_VFS_FILENAME1) || strstr(Filename, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Found: %s (%x)", Filename, Checksum);
#endif
            return LookupTableItem->PackfileEntry;
        }

        LookupTableItem = LookupTableItem->Next;
    }
    while(LookupTableItem);

    VFS_DBGPRINT("Cannot find: %s (%x)", Filename, Checksum);

    /*LookupTableItem = &g_VfsLookupTableNew[Checksum % LOOKUP_TABLE_SIZE];
    do
    {
        if(LookupTableItem->PackfileEntry)
        {
            MessageBox(0, LookupTableItem->PackfileEntry->FileName, "List", 0);
        }

        LookupTableItem = LookupTableItem->Next;
    }
    while(LookupTableItem);*/

    return NULL;
}

static void LoadDashFactionVpp()
{
    // Load DashFaction specific packfile
    char Buf[MAX_PATH];
    GetModuleFileNameA(g_hmodule, Buf, sizeof(Buf));
    char *Ptr = strrchr(Buf, '\\');
    strcpy(Ptr + 1, "dashfaction.vpp");
    if (PathFileExistsA(Buf))
        *(Ptr + 1) = '\0';
    else
    {
        // try to remove 2 path components (bin/(debug|release))
        *Ptr = 0;
        Ptr = strrchr(Buf, '\\');
        if (Ptr)
            *Ptr = 0;
        Ptr = strrchr(Buf, '\\');
        if (Ptr)
            *(Ptr + 1) = '\0';
    }
    INFO("Loading dashfaction.vpp from directory: %s", Buf);
    if (!rf::PackfileLoad("dashfaction.vpp", Buf))
        ERR("Failed to load dashfaction.vpp");
}

static void PackfileInit_New()
{
    unsigned StartTicks = GetTickCount();

    memset(g_VfsLookupTableNew, 0, sizeof(PackfileLookupTableNew) * VFS_LOOKUP_TABLE_SIZE);

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
    *((int*)0x01BDB218) = 1; // PackfilesLoaded
    *((uint32_t*)0x01BDB210) = 10000; // cFilesInVfs
    *((uint32_t*)0x01BDB214) = 100; // cPackfiles

    // Note: language changes in binary are done here to make sure RootPath is already initialized

    // Switch UI language - can be anything even if this is US edition
    WriteMem<u8>(0x004B27D2 + 1, (uint8_t)GetInstalledGameLang());

    // Switch localized tables names
    if (GetInstalledGameLang() != LANG_EN)
    {
        WriteMemPtr(0x0043DCAB + 1, "localized_credits.tbl");
        WriteMemPtr(0x0043E50B + 1, "localized_endgame.tbl");
        WriteMemPtr(0x004B082B + 1, "localized_strings.tbl");
    }

    INFO("Packfiles initialization took %dms", GetTickCount() - StartTicks);
    INFO("Packfile name collisions: %d", g_cNameCollisions);

    if (g_ModdedGame)
        INFO("Modded game detected!");
}

static void PackfileCleanup_New()
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
        free(g_Packfiles[i]);
    free(g_Packfiles);
    g_Packfiles = nullptr;
    g_cPackfiles = 0;
}

void VfsApplyHooks()
{
    AsmWritter(0x0052BCA0).jmp(PackfileAddToLookupTable_New);
    AsmWritter(0x0052BD40).jmp(PackfileAddEntries_New);
    AsmWritter(0x0052C4D0).jmp(PackfileBuildEntriesList_New);
    AsmWritter(0x0052C070).jmp(PackfileLoad_New);
    AsmWritter(0x0052C1D0).jmp(PackfileFindArchive_New);
    AsmWritter(0x0052C220).jmp(PackfileFindFileInternal_New);
    AsmWritter(0x0052BB60).jmp(PackfileInit_New);
    AsmWritter(0x0052BC80).jmp(PackfileCleanup_New);

#ifdef DEBUG
    WriteMem<u8>(0x0052BEF0, 0xFF); // VfsInitPackfileFilesList
    WriteMem<u8>(0x0052BF50, 0xFF); // VfsLoadPackfileInternal
    WriteMem<u8>(0x0052C440, 0xFF); // VfsFindPackfileEntry
#endif

    /* Load ui.vpp before tables.vpp - not used anymore */
    //WriteMemPtr(0x0052BC58, "ui.vpp");
    //WriteMemPtr(0x0052BC67, "tables.vpp");
}

void ForceFileFromPackfile(const char *Name, const char *PackfileName)
{
    rf::Packfile *Packfile = rf::PackfileFindArchive(PackfileName);
    if (Packfile)
    {
        for (unsigned i = 0; i < Packfile->cFiles; ++i)
        {
            if (!stricmp(Packfile->FileList[i].FileName, Name))
                rf::PackfileAddToLookupTable(&Packfile->FileList[i]);
        }
    }
}

void PackfileFindMatchingFiles(const StringMatcher &Query, std::function<void(const char *)> ResultConsumer)
{
    for (int i = 0; i < LOOKUP_TABLE_SIZE; ++i)
    {
        PackfileLookupTableNew *LookupTableItem = &g_VfsLookupTableNew[i];
        if (!LookupTableItem->PackfileEntry)
            continue;

        do
        {
            const char *Filename = LookupTableItem->PackfileEntry->FileName;
            if (Query(Filename))
                ResultConsumer(LookupTableItem->PackfileEntry->FileName);
            LookupTableItem = LookupTableItem->Next;
        } while (LookupTableItem);
    }
}
