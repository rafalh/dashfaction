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
    uint32_t NumFiles;
    PackfileEntry *FileList;
    uint32_t PackfileSize;
};

struct PackfileEntry
{
    uint32_t NameChecksum;
    const char *FileName;
    uint32_t OffsetInBlocks;
    uint32_t FileSize;
    Packfile *Archive;
    FILE *RawFile;
};

struct PackfileLookupTable
{
    uint32_t NameChecksum;
    PackfileEntry *ArchiveEntry;
};

static auto &g_VfsIgnoreTblFiles = AddrAsRef<bool>(0x01BDB21C);

typedef Packfile *(*PackfileFindArchive_Type)(const char *filename);
static const auto PackfileFindArchive = reinterpret_cast<PackfileFindArchive_Type>(0x0052C1D0);

typedef uint32_t(*PackfileCalcFileNameChecksum_Type)(const char *file_name);
static const auto PackfileCalcFileNameChecksum = reinterpret_cast<PackfileCalcFileNameChecksum_Type>(0x0052BE70);

typedef uint32_t(*PackfileAddToLookupTable_Type)(PackfileEntry *packfile_entry);
static const auto PackfileAddToLookupTable = reinterpret_cast<PackfileAddToLookupTable_Type>(0x0052BCA0);

typedef uint32_t(*PackfileProcessHeader_Type)(Packfile *packfile, const void *header);
static const auto PackfileProcessHeader = reinterpret_cast<PackfileProcessHeader_Type>(0x0052BD10);

typedef uint32_t(*PackfileAddEntries_Type)(Packfile *packfile, const void *buf, unsigned c_files_in_block, unsigned *pc_added_entries);
static const auto PackfileAddEntries = reinterpret_cast<PackfileAddEntries_Type>(0x0052BD40);

typedef uint32_t(*PackfileSetupFileOffsets_Type)(Packfile *packfile, unsigned data_offset_in_blocks);
static const auto PackfileSetupFileOffsets = reinterpret_cast<PackfileSetupFileOffsets_Type>(0x0052BEB0);

static const auto FileGetChecksum = AddrAsRef<unsigned(const char *filename)>(0x00436630);
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

static constexpr auto LOOKUP_TABLE_SIZE = 20713;
static auto &g_VfsLookupTableNew = *reinterpret_cast<PackfileLookupTableNew(*)[LOOKUP_TABLE_SIZE]>(0x01BB2AC8);

static unsigned g_NumFilesInVfs = 0, g_NumNameCollisions = 0;
static std::vector<rf::Packfile*> g_Packfiles;
static bool g_ModdedGame = false;

#ifdef MOD_FILE_WHITELIST

const char *ModFileWhitelist[] = {
    "reticle_0.tga",
    "scope_ret_0.tga",
    "reticle_rocket_0.tga",
};

static bool IsModFileInWhitelist(const char *Filename)
{
    for (unsigned i = 0; i < std::size(ModFileWhitelist); ++i)
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
    std::array lang_codes{"en", "gr", "fr"};

    for (unsigned i = 0; i < lang_codes.size(); ++i)
    {
        auto full_path = StringFormat("%smaps_%s.vpp", rf::g_RootPath, lang_codes[i]);
        BOOL exists = PathFileExistsA(full_path.c_str());
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
    VFS_DBGPRINT("Load packfile %s %s", dir, filename);

    std::string full_path;
    if (dir && dir[0] && dir[1] == ':')
        full_path = StringFormat("%s%s", dir, filename); // absolute path
    else
        full_path = StringFormat("%s%s%s", rf::g_RootPath, dir ? dir : "", filename);

    if (!filename || strlen(filename) > 0x1F || full_path.size() > 0x7F)
    {
        ERR("Packfile name or path too long: %s", full_path.c_str());
        return 0;
    }

    for (rf::Packfile *packfile : g_Packfiles)
        if (!stricmp(packfile->Path, full_path.c_str()))
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

    FILE *file = fopen(full_path.c_str(), "rb");
    if (!file)
    {
        ERR("Failed to open packfile %s", full_path);
        return 0;
    }

    rf::Packfile *packfile = new rf::Packfile;
    strncpy(packfile->Name, filename, sizeof(packfile->Name) - 1);
    packfile->Name[sizeof(packfile->Name) - 1] = '\0';
    strncpy(packfile->Path, full_path.c_str(), sizeof(packfile->Path) - 1);
    packfile->Path[sizeof(packfile->Path) - 1] = '\0';
    packfile->field_A0 = 0;
    packfile->NumFiles = 0;
    packfile->FileList = nullptr;

    // Note: VfsProcessPackfileHeader returns number of files in packfile - result 0 is not always a true error
    int ret = 0;
    unsigned offset_in_blocks = 1;
    char buf[0x800];
    if (fread(buf, sizeof(buf), 1, file) == 1 && rf::PackfileProcessHeader(packfile, buf))
    {
        packfile->FileList = new rf::PackfileEntry[packfile->NumFiles];
        memset(packfile->FileList, 0, packfile->NumFiles * sizeof(rf::PackfileEntry));
        unsigned num_added = 0;
        ret = 1;

        for (unsigned i = 0; i < packfile->NumFiles; i += 32)
        {
            if (fread(buf, sizeof(buf), 1, file) != 1)
            {
                ret = 0;
                ERR("Failed to fread vpp %s", full_path);
                break;
            }

            unsigned num_files_in_block = std::min(packfile->NumFiles - i, 32u);
            rf::PackfileAddEntries(packfile, buf, num_files_in_block, &num_added);
            ++offset_in_blocks;
        }
    } else ERR("Failed to fread vpp 2 %s", full_path);

    if (ret)
        rf::PackfileSetupFileOffsets(packfile, offset_in_blocks);

    fclose(file);

    if (ret)
    {
        g_Packfiles.push_back(packfile);
    }
    else
    {
        delete[] packfile->FileList;
        delete packfile;
    }

    return ret;
}

static rf::Packfile *PackfileFindArchive_New(const char *filename)
{
    for (auto packfile : g_Packfiles)
    {
        if (!stricmp(packfile->Name, filename))
            return packfile;
    }

    ERR("Packfile %s not found", filename);
    return nullptr;
}

static int PackfileBuildEntriesList_New(const char *ext_list, char *&filenames, unsigned &num_files, const char *packfile_name)
{
    unsigned num_bytes = 1;

    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    num_files = 0;
    filenames = 0;

    for (rf::Packfile *packfile : g_Packfiles)
    {
        if (!packfile_name || !stricmp(packfile_name, packfile->Name))
        {
            for (unsigned j = 0; j < packfile->NumFiles; ++j)
            {
                const char *ext = rf::GetFileExt(packfile->FileList[j].FileName);
                if (ext[0] && strstr(ext_list, ext + 1))
                    num_bytes += strlen(packfile->FileList[j].FileName) + 1;
            }
        }
    }

    filenames = (char*)rf::Malloc(num_bytes);
    if (!filenames)
        return 0;
    char *buf_ptr = filenames;
    for (rf::Packfile *packfile : g_Packfiles)
    {
        if (!packfile_name || !stricmp(packfile_name, packfile->Name))
        {
            for (unsigned j = 0; j < packfile->NumFiles; ++j)
            {
                const char *ext = rf::GetFileExt(packfile->FileList[j].FileName);
                if (ext[0] && strstr(ext_list, ext + 1))
                {
                    strcpy(buf_ptr, packfile->FileList[j].FileName);
                    buf_ptr += strlen(packfile->FileList[j].FileName) + 1;
                    ++num_files;
                }
            }

        }
    }
    // terminating zero
    buf_ptr[0] = 0;

    return 1;
}

static int PackfileAddEntries_New(rf::Packfile *packfile, const void *block, unsigned num_files, unsigned &num_added_files)
{
    const uint8_t *data = (const uint8_t*)block;

    for (unsigned i = 0; i < num_files; ++i)
    {
        auto file_name = reinterpret_cast<const char*>(data);
        rf::PackfileEntry &entry = packfile->FileList[num_added_files];
        if (rf::g_VfsIgnoreTblFiles && !stricmp(rf::GetFileExt((char*)data), ".tbl"))
            entry.FileName = "DEADBEEF";
        else
        {
            // FIXME: free?
            char *file_name_buf = new char[strlen(file_name) + 10];
            memset(file_name_buf, 0, strlen(file_name) + 10);
            strcpy(file_name_buf, file_name);
            entry.FileName = file_name_buf;
        }
        entry.NameChecksum = rf::PackfileCalcFileNameChecksum(entry.FileName);
        entry.FileSize = *reinterpret_cast<const uint32_t*>(data + 60);
        entry.Archive = packfile;
        entry.RawFile = nullptr;

        data += 64;
        ++num_added_files;

        rf::PackfileAddToLookupTable(&entry);
        ++g_NumFilesInVfs;
    }
    return 1;
}

static void PackfileAddToLookupTable_New(rf::PackfileEntry *entry)
{
    PackfileLookupTableNew *lookup_table_item = &g_VfsLookupTableNew[entry->NameChecksum % LOOKUP_TABLE_SIZE];

    while (true)
    {
        if (!lookup_table_item->PackfileEntry)
        {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 1: %s (%x)", Entry->FileName, Entry->NameChecksum);
#endif
            break;
        }

        if (!stricmp(lookup_table_item->PackfileEntry->FileName, entry->FileName))
        {
            // file with the same name already exist
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->FileName, DEBUG_VFS_FILENAME1) || !stricmp(Entry->FileName, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 2: %s (%x)", Entry->FileName, Entry->NameChecksum);
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
                VFS_DBGPRINT("Add 3: %s (%x)", Entry->FileName, Entry->NameChecksum);
#endif

            lookup_table_item->Next = new PackfileLookupTableNew;
            lookup_table_item = lookup_table_item->Next;
            lookup_table_item->Next = nullptr;
            break;
        }

        lookup_table_item = lookup_table_item->Next;
        ++g_NumNameCollisions;
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
           lookup_table_item->PackfileEntry->NameChecksum == checksum &&
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
    while (lookup_table_item);

    VFS_DBGPRINT("Cannot find: %s (%x)", filename, checksum);

    return nullptr;
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
        // try to remove 3 path components (build/(debug|release)/bin)
        *ptr = '\0';
        ptr = strrchr(buf, '\\');
        if (ptr)
            *ptr = '\0';
        ptr = strrchr(buf, '\\');
        if (ptr)
            *ptr = '\0';
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

    memset(g_VfsLookupTableNew, 0, sizeof(PackfileLookupTableNew) * LOOKUP_TABLE_SIZE);

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
    INFO("Packfile name collisions: %d", g_NumNameCollisions);

    if (g_ModdedGame)
        INFO("Modded game detected!");
}

static void PackfileCleanup_New()
{
    for (rf::Packfile *packfile : g_Packfiles)
    {
        delete[] packfile->FileList;
        delete packfile;
    }
    g_Packfiles.clear();
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
}

void ForceFileFromPackfile(const char *name, const char *packfile_name)
{
    rf::Packfile *packfile = rf::PackfileFindArchive(packfile_name);
    if (packfile)
    {
        for (unsigned i = 0; i < packfile->NumFiles; ++i)
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
