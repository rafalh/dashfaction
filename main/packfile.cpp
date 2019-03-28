#include "stdafx.h"
#include "packfile.h"
#include "utils.h"
#include "rf.h"
#include "main.h"
#include "xxhash.h"
#include <shlwapi.h>
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

typedef Packfile *(*PackfileFindArchive_Type)(const char *filename);
static const auto PackfileFindArchive = (PackfileFindArchive_Type)0x0052C1D0;

typedef uint32_t(*PackfileCalcFileNameChecksum_Type)(const char *file_name);
static const auto PackfileCalcFileNameChecksum = (PackfileCalcFileNameChecksum_Type)0x0052BE70;

typedef uint32_t(*PackfileAddToLookupTable_Type)(PackfileEntry *packfile_entry);
static const auto PackfileAddToLookupTable = (PackfileAddToLookupTable_Type)0x0052BCA0;

typedef uint32_t(*PackfileProcessHeader_Type)(Packfile *packfile, const void *header);
static const auto PackfileProcessHeader = (PackfileProcessHeader_Type)0x0052BD10;

typedef uint32_t(*PackfileAddEntries_Type)(Packfile *packfile, const void *buf, unsigned c_files_in_block, unsigned *pc_added_entries);
static const auto PackfileAddEntries = (PackfileAddEntries_Type)0x0052BD40;

typedef uint32_t(*PackfileSetupFileOffsets_Type)(Packfile *packfile, unsigned data_offset_in_blocks);
static const auto PackfileSetupFileOffsets = (PackfileSetupFileOffsets_Type)0x0052BEB0;

static const auto FileGetChecksum = (unsigned(*)(const char *filename))0x00436630;
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
    char full_path[MAX_PATH];
    const char *lang_codes[] = { "en", "gr", "fr" };
    for (unsigned i = 0; i < COUNTOF(lang_codes); ++i)
    {
        sprintf(full_path, "%smaps_%s.vpp", rf::g_RootPath, lang_codes[i]);
        BOOL exists = PathFileExistsA(full_path);
        INFO("Checking file %s: %s", full_path, exists ? "found" : "not found");
        if (exists)
        {
            INFO("Detected game language: %s", lang_codes[i]);
            return (GameLang)i;
        }
    }
    WARN("Cannot detect game language");
    return LANG_EN; // default
}

GameLang GetInstalledGameLang()
{
    static bool initialized = false;
    static GameLang installed_game_lang;
    if (!initialized)
    {
        installed_game_lang = DetectInstalledGameLang();
        initialized = true;
    }
    return installed_game_lang;
}

bool IsModdedGame()
{
    return g_ModdedGame;
}

static int PackfileLoad_New(const char *filename, const char *dir)
{
    char full_path[256], buf[0x800];
    int ret = 0;
    unsigned c_files_in_block, c_added, offset_in_blocks;

    VFS_DBGPRINT("Load packfile %s %s", dir, filename);

    if (dir && dir[0] && dir[1] == ':')
        sprintf(full_path, "%s%s", dir, filename); // absolute path
    else
        sprintf(full_path, "%s%s%s", rf::g_RootPath, dir ? dir : "", filename);

    if (!filename || strlen(filename) > 0x1F || strlen(full_path) > 0x7F)
    {
        ERR("Packfile name or path too long: %s", full_path);
        return 0;
    }

    for (unsigned i = 0; i < g_cPackfiles; ++i)
        if (!stricmp(g_Packfiles[i]->Path, full_path))
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

    FILE *file = fopen(full_path, "rb");
    if (!file)
    {
        ERR("Failed to open packfile %s", full_path);
        return 0;
    }

    if (g_cPackfiles % 64 == 0)
    {
        g_Packfiles = (rf::Packfile**)realloc(g_Packfiles, (g_cPackfiles + 64) * sizeof(rf::Packfile*));
        memset(&g_Packfiles[g_cPackfiles], 0, 64 * sizeof(rf::Packfile*));
    }

    rf::Packfile *packfile = (rf::Packfile*)malloc(sizeof(*packfile));
    if (!packfile)
    {
        ERR("malloc failed");
        return 0;
    }

    strncpy(packfile->Name, filename, sizeof(packfile->Name) - 1);
    packfile->Name[sizeof(packfile->Name) - 1] = '\0';
    strncpy(packfile->Path, full_path, sizeof(packfile->Path) - 1);
    packfile->Path[sizeof(packfile->Path) - 1] = '\0';
    packfile->field_A0 = 0;
    packfile->cFiles = 0;
    packfile->FileList = NULL;

    // Note: VfsProcessPackfileHeader returns number of files in packfile - result 0 is not always a true error
    if (fread(buf, sizeof(buf), 1, file) == 1 && rf::PackfileProcessHeader(packfile, buf))
    {
        packfile->FileList = (rf::PackfileEntry*)malloc(packfile->cFiles * sizeof(rf::PackfileEntry));
        memset(packfile->FileList, 0, packfile->cFiles * sizeof(rf::PackfileEntry));
        c_added = 0;
        offset_in_blocks = 1;
        ret = 1;

        for (unsigned i = 0; i < packfile->cFiles; i += 32)
        {
            if (fread(buf, sizeof(buf), 1, file) != 1)
            {
                ret = 0;
                ERR("Failed to fread vpp %s", full_path);
                break;
            }

            c_files_in_block = std::min(packfile->cFiles - i, 32u);
            rf::PackfileAddEntries(packfile, buf, c_files_in_block, &c_added);
            ++offset_in_blocks;
        }
    } else ERR("Failed to fread vpp 2 %s", full_path);

    if (ret)
        rf::PackfileSetupFileOffsets(packfile, offset_in_blocks);

    fclose(file);

    if (ret)
    {
        g_Packfiles[g_cPackfiles] = packfile;
        ++g_cPackfiles;
    }
    else
    {
        free(packfile->FileList);
        free(packfile);
    }

    return ret;
}

static rf::Packfile *PackfileFindArchive_New(const char *filename)
{
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        if (!stricmp(g_Packfiles[i]->Name, filename))
            return g_Packfiles[i];
    }

    ERR("Packfile %s not found", filename);
    return NULL;
}

static int PackfileBuildEntriesList_New(const char *ext_list, char **pp_filenames, unsigned *pc_files, const char *packfile_name)
{
    unsigned cb_buf = 1;

    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    *pc_files = 0;
    *pp_filenames = 0;

    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        rf::Packfile *packfile = g_Packfiles[i];
        if (!packfile_name || !stricmp(packfile_name, packfile->Name))
        {
            for (unsigned j = 0; j < packfile->cFiles; ++j)
            {
                const char *ext = rf::GetFileExt(packfile->FileList[j].FileName);
                if (ext[0] && strstr(ext_list, ext + 1))
                    cb_buf += strlen(packfile->FileList[j].FileName) + 1;
            }
        }
    }

    *pp_filenames = (char*)rf::Malloc(cb_buf);
    if (!*pp_filenames)
        return 0;
    char *buf_ptr = *pp_filenames;
    for (unsigned i = 0; i < g_cPackfiles; ++i)
    {
        rf::Packfile *packfile = g_Packfiles[i];
        if (!packfile_name || !stricmp(packfile_name, packfile->Name))
        {
            for (unsigned j = 0; j < packfile->cFiles; ++j)
            {
                const char *ext = rf::GetFileExt(packfile->FileList[j].FileName);
                if (ext[0] && strstr(ext_list, ext + 1))
                {
                    strcpy(buf_ptr, packfile->FileList[j].FileName);
                    buf_ptr += strlen(packfile->FileList[j].FileName) + 1;
                    ++(*pc_files);
                }
            }

        }
    }
    // terminating zero
    buf_ptr[0] = 0;

    return 1;
}

static int PackfileAddEntries_New(rf::Packfile *packfile, const void *block, unsigned c_files_in_block, unsigned *pc_added_files)
{
    const uint8_t *data = (const uint8_t*)block;

    for (unsigned i = 0; i < c_files_in_block; ++i)
    {
        rf::PackfileEntry *entry = &packfile->FileList[*pc_added_files];
        if (rf::g_VfsIgnoreTblFiles && !stricmp(rf::GetFileExt((char*)data), ".tbl"))
            entry->FileName = "DEADBEEF";
        else
        {
            // FIXME: free?
            char *file_name_buf = (char*)malloc(strlen((char*)data) + 10);
            memset(file_name_buf, 0, strlen((char*)data) + 10);
            strcpy(file_name_buf, (char*)data);
            entry->FileName = file_name_buf;
        }
        entry->dwNameChecksum = rf::PackfileCalcFileNameChecksum(entry->FileName);
        entry->cbFileSize = *((uint32_t*)(data + 60));
        entry->Archive = packfile;
        entry->RawFile = NULL;

        data += 64;
        ++(*pc_added_files);

        rf::PackfileAddToLookupTable(entry);
        ++g_cFilesInVfs;
    }
    return 1;
}

static void PackfileAddToLookupTable_New(rf::PackfileEntry *entry)
{
    PackfileLookupTableNew *lookup_table_item = &g_VfsLookupTableNew[entry->dwNameChecksum % LOOKUP_TABLE_SIZE];

    while (true)
    {
        if (!lookup_table_item->PackfileEntry)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 1: %s (%x)", Entry->FileName, Entry->dwNameChecksum);
#endif
            break;
        }

        if (!stricmp(lookup_table_item->PackfileEntry->FileName, entry->FileName))
        {
            // file with the same name already exist
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 2: %s (%x)", Entry->FileName, Entry->dwNameChecksum);
#endif

            const char *old_archive = lookup_table_item->PackfileEntry->Archive->Name;
            const char *new_archive = entry->Archive->Name;

            if (rf::g_VfsIgnoreTblFiles) // this is set to true for user_maps
            {
                bool whitelisted = false;
#ifdef MOD_FILE_WHITELIST
                Whitelisted = IsModFileInWhitelist(Entry->FileName);
#endif
                if (!g_game_config.allowOverwriteGameFiles && !whitelisted)
                {
                    TRACE("Denied overwriting game file %s (old packfile %s, new packfile %s)",
                        entry->FileName, old_archive, new_archive);
                    return;
                }
                else
                {
                    TRACE("Allowed overwriting game file %s (old packfile %s, new packfile %s)",
                        entry->FileName, old_archive, new_archive);
                    if (!whitelisted)
                        g_ModdedGame = true;
                }
            }
            else
            {
                TRACE("Overwriting packfile item %s (old packfile %s, new packfile %s)",
                    entry->FileName, old_archive, new_archive);
            }
            break;
        }

        if (!lookup_table_item->Next)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 3: %s (%x)", Entry->FileName, Entry->dwNameChecksum);
#endif

            lookup_table_item->Next = (PackfileLookupTableNew*)malloc(sizeof(PackfileLookupTableNew));
            lookup_table_item = lookup_table_item->Next;
            lookup_table_item->Next = NULL;
            break;
        }

        lookup_table_item = lookup_table_item->Next;
        ++g_cNameCollisions;
    }

    lookup_table_item->PackfileEntry = entry;
}

static rf::PackfileEntry *PackfileFindFileInternal_New(const char *filename)
{
    unsigned checksum = rf::PackfileCalcFileNameChecksum(filename);
    PackfileLookupTableNew *lookup_table_item = &g_VfsLookupTableNew[checksum % LOOKUP_TABLE_SIZE];
    do
    {
        if (lookup_table_item->PackfileEntry &&
           lookup_table_item->PackfileEntry->dwNameChecksum == checksum &&
           !stricmp(lookup_table_item->PackfileEntry->FileName, filename))
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (strstr(Filename, DEBUG_VFS_FILENAME1) || strstr(Filename, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Found: %s (%x)", Filename, Checksum);
#endif
            return lookup_table_item->PackfileEntry;
        }

        lookup_table_item = lookup_table_item->Next;
    }
    while(lookup_table_item);

    VFS_DBGPRINT("Cannot find: %s (%x)", filename, checksum);

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
    char buf[MAX_PATH];
    GetModuleFileNameA(g_hmodule, buf, sizeof(buf));
    char *ptr = strrchr(buf, '\\');
    strcpy(ptr + 1, "dashfaction.vpp");
    if (PathFileExistsA(buf))
        *(ptr + 1) = '\0';
    else
    {
        // try to remove 2 path components (bin/(debug|release))
        *ptr = 0;
        ptr = strrchr(buf, '\\');
        if (ptr)
            *ptr = 0;
        ptr = strrchr(buf, '\\');
        if (ptr)
            *(ptr + 1) = '\0';
    }
    INFO("Loading dashfaction.vpp from directory: %s", buf);
    if (!rf::PackfileLoad("dashfaction.vpp", buf))
        ERR("Failed to load dashfaction.vpp");
}

static void PackfileInit_New()
{
    unsigned start_ticks = GetTickCount();

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

    INFO("Packfiles initialization took %dms", GetTickCount() - start_ticks);
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

void ForceFileFromPackfile(const char *name, const char *packfile_name)
{
    rf::Packfile *packfile = rf::PackfileFindArchive(packfile_name);
    if (packfile)
    {
        for (unsigned i = 0; i < packfile->cFiles; ++i)
        {
            if (!stricmp(packfile->FileList[i].FileName, name))
                rf::PackfileAddToLookupTable(&packfile->FileList[i]);
        }
    }
}

void PackfileFindMatchingFiles(const StringMatcher &query, std::function<void(const char *)> result_consumer)
{
    for (int i = 0; i < LOOKUP_TABLE_SIZE; ++i)
    {
        PackfileLookupTableNew *lookup_table_item = &g_VfsLookupTableNew[i];
        if (!lookup_table_item->PackfileEntry)
            continue;

        do
        {
            const char *filename = lookup_table_item->PackfileEntry->FileName;
            if (query(filename))
                result_consumer(lookup_table_item->PackfileEntry->FileName);
            lookup_table_item = lookup_table_item->Next;
        } while (lookup_table_item);
    }
}
