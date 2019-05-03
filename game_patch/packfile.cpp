#include "packfile.h"
#include "main.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include "xxhash.h"
#include <patch_common/ShortTypes.h>
#include <array>
#include <shlwapi.h>

#define DEBUG_VFS 0
#define DEBUG_VFS_FILENAME1 "DM RTS MiniGolf 2.1.rfl"
#define DEBUG_VFS_FILENAME2 "JumpPad.wav"

#define VFS_DBGPRINT TRACE

namespace rf
{
struct PackfileEntry;

struct Packfile
{
    char name[32];
    char path[128];
    uint32_t field_a0;
    uint32_t num_files;
    PackfileEntry* file_list;
    uint32_t packfile_size;
};

struct PackfileEntry
{
    uint32_t name_checksum;
    const char* file_name;
    uint32_t offset_in_blocks;
    uint32_t file_size;
    Packfile* archive;
    FILE* raw_file;
};

struct PackfileLookupTable
{
    uint32_t name_checksum;
    PackfileEntry* archive_entry;
};

static auto& packfile_ignore_tbl_files = AddrAsRef<bool>(0x01BDB21C);

static auto& PackfileFindArchive = AddrAsRef<Packfile*(const char* filename)>(0x0052C1D0);
static auto& PackfileCalcFileNameChecksum = AddrAsRef<uint32_t(const char* file_name)>(0x0052BE70);
static auto& PackfileAddToLookupTable = AddrAsRef<uint32_t(PackfileEntry* packfile_entry)>(0x0052BCA0);
static auto& PackfileProcessHeader = AddrAsRef<uint32_t(Packfile* packfile, const void* header)>(0x0052BD10);
typedef uint32_t PackfileAddEntries_Type(Packfile* packfile, const void* buf, unsigned num_files_in_block,
                                         unsigned* num_added);
static auto& PackfileAddEntries = AddrAsRef<PackfileAddEntries_Type>(0x0052BD40);
static auto& PackfileSetupFileOffsets =
    AddrAsRef<uint32_t(Packfile* packfile, unsigned data_offset_in_blocks)>(0x0052BEB0);
} // namespace rf

struct PackfileLookupTableNew
{
    PackfileLookupTableNew* next;
    rf::PackfileEntry* packfile_entry;
};

#if CHECK_PACKFILE_CHECKSUM

const std::map<std::string, unsigned> GameFileChecksums = {
    // Note: Multiplayer level checksum is checked when loading level
    //{ "levelsm.vpp", 0x17D0D38A },
    // Note: maps are big and slow downs startup
    {"maps1.vpp", 0x52EE4F99},  {"maps2.vpp", 0xB053486F},   {"maps3.vpp", 0xA5ED6271},  {"maps4.vpp", 0xE0AB4397},
    {"meshes.vpp", 0xEBA19172}, {"motions.vpp", 0x17132D8E}, {"tables.vpp", 0x549DAABF},
};

#endif // CHECK_PACKFILE_CHECKSUM

static constexpr auto LOOKUP_TABLE_SIZE = 20713;
static auto& g_packfile_lookup_table = AddrAsRef<PackfileLookupTableNew[LOOKUP_TABLE_SIZE]>(0x01BB2AC8);

static unsigned g_num_files_in_packfiles = 0, g_NumNameCollisions = 0;
static std::vector<rf::Packfile*> g_packfiles;
static bool g_is_modded_game = false;

#ifdef MOD_FILE_WHITELIST

const char* ModFileWhitelist[] = {
    "reticle_0.tga",
    "scope_ret_0.tga",
    "reticle_rocket_0.tga",
};

static bool IsModFileInWhitelist(const char* Filename)
{
    for (unsigned i = 0; i < std::size(ModFileWhitelist); ++i)
        if (!stricmp(ModFileWhitelist[i], Filename))
            return true;
    return false;
}

#endif // MOD_FILE_WHITELIST

#if CHECK_PACKFILE_CHECKSUM

static unsigned HashFile(const char* Filename)
{
    FILE* File = fopen(Filename, "rb");
    if (!File)
        return 0;

    XXH32_state_t* state = XXH32_createState();
    XXH32_reset(state, 0);

    char Buffer[4096];
    while (true) {
        size_t len = fread(Buffer, 1, sizeof(Buffer), File);
        if (!len)
            break;
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

    for (unsigned i = 0; i < lang_codes.size(); ++i) {
        auto full_path = StringFormat("%smaps_%s.vpp", rf::root_path, lang_codes[i]);
        BOOL exists = PathFileExistsA(full_path.c_str());
        INFO("Checking file %s: %s", full_path.c_str(), exists ? "found" : "not found");
        if (exists) {
            INFO("Detected game language: %s", lang_codes[i]);
            return static_cast<GameLang>(i);
        }
    }
    WARN("Cannot detect game language");
    return LANG_EN; // default
}

GameLang GetInstalledGameLang()
{
    static bool initialized = false;
    static GameLang installed_game_lang;
    if (!initialized) {
        installed_game_lang = DetectInstalledGameLang();
        initialized = true;
    }
    return installed_game_lang;
}

bool IsModdedGame()
{
    return g_is_modded_game;
}

static int PackfileLoad_New(const char* filename, const char* dir)
{
    VFS_DBGPRINT("Load packfile %s %s", dir, filename);

    std::string full_path;
    if (dir && dir[0] && dir[1] == ':')
        full_path = StringFormat("%s%s", dir, filename); // absolute path
    else
        full_path = StringFormat("%s%s%s", rf::root_path, dir ? dir : "", filename);

    if (!filename || strlen(filename) > 0x1F || full_path.size() > 0x7F) {
        ERR("Packfile name or path too long: %s", full_path.c_str());
        return 0;
    }

    for (rf::Packfile* packfile : g_packfiles)
        if (!stricmp(packfile->path, full_path.c_str()))
            return 1;

#if CHECK_PACKFILE_CHECKSUM
    if (!Dir) {
        auto it = GameFileChecksums.find(Filename);
        if (it != GameFileChecksums.end()) {
            unsigned Checksum = HashFile(FullPath);
            if (Checksum != it->second) {
                INFO("Packfile %s has invalid checksum 0x%X", Filename, Checksum);
                g_is_modded_game = true;
            }
        }
    }
#endif // CHECK_PACKFILE_CHECKSUM

    FILE* file = fopen(full_path.c_str(), "rb");
    if (!file) {
        ERR("Failed to open packfile %s", full_path.c_str());
        return 0;
    }

    rf::Packfile* packfile = new rf::Packfile;
    strncpy(packfile->name, filename, sizeof(packfile->name) - 1);
    packfile->name[sizeof(packfile->name) - 1] = '\0';
    strncpy(packfile->path, full_path.c_str(), sizeof(packfile->path) - 1);
    packfile->path[sizeof(packfile->path) - 1] = '\0';
    packfile->field_a0 = 0;
    packfile->num_files = 0;
    packfile->file_list = nullptr;

    // Note: VfsProcessPackfileHeader returns number of files in packfile - result 0 is not always a true error
    int ret = 0;
    unsigned offset_in_blocks = 1;
    char buf[0x800];
    if (fread(buf, sizeof(buf), 1, file) == 1 && rf::PackfileProcessHeader(packfile, buf)) {
        packfile->file_list = new rf::PackfileEntry[packfile->num_files];
        memset(packfile->file_list, 0, packfile->num_files * sizeof(rf::PackfileEntry));
        unsigned num_added = 0;
        ret = 1;

        for (unsigned i = 0; i < packfile->num_files; i += 32) {
            if (fread(buf, sizeof(buf), 1, file) != 1) {
                ret = 0;
                ERR("Failed to fread vpp %s", full_path.c_str());
                break;
            }

            unsigned num_files_in_block = std::min(packfile->num_files - i, 32u);
            rf::PackfileAddEntries(packfile, buf, num_files_in_block, &num_added);
            ++offset_in_blocks;
        }
    }
    else
        ERR("Failed to fread vpp 2 %s", full_path.c_str());

    if (ret)
        rf::PackfileSetupFileOffsets(packfile, offset_in_blocks);

    fclose(file);

    if (ret) {
        g_packfiles.push_back(packfile);
    }
    else {
        delete[] packfile->file_list;
        delete packfile;
    }

    return ret;
}

static rf::Packfile* PackfileFindArchive_New(const char* filename)
{
    for (auto packfile : g_packfiles) {
        if (!stricmp(packfile->name, filename))
            return packfile;
    }

    ERR("Packfile %s not found", filename);
    return nullptr;
}

static int PackfileBuildEntriesList_New(const char* ext_list, char*& filenames, unsigned& num_files,
                                        const char* packfile_name)
{
    unsigned num_bytes = 1;

    VFS_DBGPRINT("VfsBuildPackfileEntriesListHook called");
    num_files = 0;
    filenames = 0;

    for (rf::Packfile* packfile : g_packfiles) {
        if (!packfile_name || !stricmp(packfile_name, packfile->name)) {
            for (unsigned j = 0; j < packfile->num_files; ++j) {
                const char* ext = rf::GetFileExt(packfile->file_list[j].file_name);
                if (ext[0] && strstr(ext_list, ext + 1))
                    num_bytes += strlen(packfile->file_list[j].file_name) + 1;
            }
        }
    }

    filenames = static_cast<char*>(rf::Malloc(num_bytes));
    if (!filenames)
        return 0;
    char* buf_ptr = filenames;
    for (rf::Packfile* packfile : g_packfiles) {
        if (!packfile_name || !stricmp(packfile_name, packfile->name)) {
            for (unsigned j = 0; j < packfile->num_files; ++j) {
                const char* ext = rf::GetFileExt(packfile->file_list[j].file_name);
                if (ext[0] && strstr(ext_list, ext + 1)) {
                    strcpy(buf_ptr, packfile->file_list[j].file_name);
                    buf_ptr += strlen(packfile->file_list[j].file_name) + 1;
                    ++num_files;
                }
            }
        }
    }
    // terminating zero
    buf_ptr[0] = 0;

    return 1;
}

static int PackfileAddEntries_New(rf::Packfile* packfile, const void* block, unsigned num_files,
                                  unsigned& num_added_files)
{
    const uint8_t* data = (const uint8_t*)block;

    for (unsigned i = 0; i < num_files; ++i) {
        auto file_name = reinterpret_cast<const char*>(data);
        rf::PackfileEntry& entry = packfile->file_list[num_added_files];
        if (rf::packfile_ignore_tbl_files && !stricmp(rf::GetFileExt(file_name), ".tbl"))
            entry.file_name = "DEADBEEF";
        else {
            // FIXME: free?
            char* file_name_buf = new char[strlen(file_name) + 10];
            memset(file_name_buf, 0, strlen(file_name) + 10);
            strcpy(file_name_buf, file_name);
            entry.file_name = file_name_buf;
        }
        entry.name_checksum = rf::PackfileCalcFileNameChecksum(entry.file_name);
        entry.file_size = *reinterpret_cast<const uint32_t*>(data + 60);
        entry.archive = packfile;
        entry.raw_file = nullptr;

        data += 64;
        ++num_added_files;

        rf::PackfileAddToLookupTable(&entry);
        ++g_num_files_in_packfiles;
    }
    return 1;
}

static void PackfileAddToLookupTable_New(rf::PackfileEntry* entry)
{
    PackfileLookupTableNew* lookup_table_item = &g_packfile_lookup_table[entry->name_checksum % LOOKUP_TABLE_SIZE];

    while (true) {
        if (!lookup_table_item->packfile_entry) {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->file_name, DEBUG_VFS_FILENAME1) || !stricmp(Entry->file_name, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 1: %s (%x)", Entry->file_name, Entry->name_checksum);
#endif
            break;
        }

        if (!stricmp(lookup_table_item->packfile_entry->file_name, entry->file_name)) {
            // file with the same name already exist
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->file_name, DEBUG_VFS_FILENAME1) || !stricmp(Entry->file_name, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 2: %s (%x)", Entry->file_name, Entry->name_checksum);
#endif

            const char* old_archive = lookup_table_item->packfile_entry->archive->name;
            const char* new_archive = entry->archive->name;

            if (rf::packfile_ignore_tbl_files) // this is set to true for user_maps
            {
                bool whitelisted = false;
#ifdef MOD_FILE_WHITELIST
                Whitelisted = IsModFileInWhitelist(Entry->file_name);
#endif
                if (!g_game_config.allowOverwriteGameFiles && !whitelisted) {
                    TRACE("Denied overwriting game file %s (old packfile %s, new packfile %s)", entry->file_name,
                          old_archive, new_archive);
                    return;
                }
                else {
                    TRACE("Allowed overwriting game file %s (old packfile %s, new packfile %s)", entry->file_name,
                          old_archive, new_archive);
                    if (!whitelisted)
                        g_is_modded_game = true;
                }
            }
            else {
                TRACE("Overwriting packfile item %s (old packfile %s, new packfile %s)", entry->file_name, old_archive,
                      new_archive);
            }
            break;
        }

        if (!lookup_table_item->next) {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (!stricmp(Entry->file_name, DEBUG_VFS_FILENAME1) || !stricmp(Entry->file_name, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Add 3: %s (%x)", Entry->file_name, Entry->name_checksum);
#endif

            lookup_table_item->next = new PackfileLookupTableNew;
            lookup_table_item = lookup_table_item->next;
            lookup_table_item->next = nullptr;
            break;
        }

        lookup_table_item = lookup_table_item->next;
        ++g_NumNameCollisions;
    }

    lookup_table_item->packfile_entry = entry;
}

static rf::PackfileEntry* PackfileFindFileInternal_New(const char* filename)
{
    unsigned checksum = rf::PackfileCalcFileNameChecksum(filename);
    PackfileLookupTableNew* lookup_table_item = &g_packfile_lookup_table[checksum % LOOKUP_TABLE_SIZE];
    do {
        if (lookup_table_item->packfile_entry && lookup_table_item->packfile_entry->name_checksum == checksum &&
            !stricmp(lookup_table_item->packfile_entry->file_name, filename)) {
#if DEBUG_VFS && defined(DEBUG_VFS_FILENAME1) && defined(DEBUG_VFS_FILENAME2)
            if (strstr(Filename, DEBUG_VFS_FILENAME1) || strstr(Filename, DEBUG_VFS_FILENAME2))
                VFS_DBGPRINT("Found: %s (%x)", Filename, Checksum);
#endif
            return lookup_table_item->packfile_entry;
        }

        lookup_table_item = lookup_table_item->next;
    } while (lookup_table_item);

    VFS_DBGPRINT("Cannot find: %s (%x)", filename, checksum);

    return nullptr;
}

static void LoadDashFactionVpp()
{
    // Load DashFaction specific packfile
    char buf[MAX_PATH];
    GetModuleFileNameA(g_hmodule, buf, sizeof(buf));
    char* ptr = strrchr(buf, '\\');
    strcpy(ptr + 1, "dashfaction.vpp");
    if (PathFileExistsA(buf))
        *(ptr + 1) = '\0';
    else {
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

    memset(g_packfile_lookup_table, 0, sizeof(PackfileLookupTableNew) * LOOKUP_TABLE_SIZE);

    if (GetInstalledGameLang() == LANG_GR) {
        rf::PackfileLoad("audiog.vpp", nullptr);
        rf::PackfileLoad("maps_gr.vpp", nullptr);
        rf::PackfileLoad("levels1g.vpp", nullptr);
        rf::PackfileLoad("levels2g.vpp", nullptr);
        rf::PackfileLoad("levels3g.vpp", nullptr);
        rf::PackfileLoad("ltables.vpp", nullptr);
    }
    else if (GetInstalledGameLang() == LANG_FR) {
        rf::PackfileLoad("audiof.vpp", nullptr);
        rf::PackfileLoad("maps_fr.vpp", nullptr);
        rf::PackfileLoad("levels1f.vpp", nullptr);
        rf::PackfileLoad("levels2f.vpp", nullptr);
        rf::PackfileLoad("levels3f.vpp", nullptr);
        rf::PackfileLoad("ltables.vpp", nullptr);
    }
    else {
        rf::PackfileLoad("audio.vpp", nullptr);
        rf::PackfileLoad("maps_en.vpp", nullptr);
        rf::PackfileLoad("levels1.vpp", nullptr);
        rf::PackfileLoad("levels2.vpp", nullptr);
        rf::PackfileLoad("levels3.vpp", nullptr);
    }
    rf::PackfileLoad("levelsm.vpp", nullptr);
    // rf::PackfileLoad("levelseb.vpp", nullptr);
    // rf::PackfileLoad("levelsbg.vpp", nullptr);
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
    AddrAsRef<int>(0x01BDB218) = 1;          // PackfilesLoaded
    AddrAsRef<uint32_t>(0x01BDB210) = 10000; // NumFilesInVfs
    AddrAsRef<uint32_t>(0x01BDB214) = 100;   // NumPackfiles

    // Note: language changes in binary are done here to make sure RootPath is already initialized

    // Switch UI language - can be anything even if this is US edition
    WriteMem<u8>(0x004B27D2 + 1, static_cast<uint8_t>(GetInstalledGameLang()));

    // Switch localized tables names
    if (GetInstalledGameLang() != LANG_EN) {
        WriteMemPtr(0x0043DCAB + 1, "localized_credits.tbl");
        WriteMemPtr(0x0043E50B + 1, "localized_endgame.tbl");
        WriteMemPtr(0x004B082B + 1, "localized_strings.tbl");
    }

    INFO("Packfiles initialization took %lums", GetTickCount() - start_ticks);
    INFO("Packfile name collisions: %d", g_NumNameCollisions);

    if (g_is_modded_game)
        INFO("Modded game detected!");
}

static void PackfileCleanup_New()
{
    for (rf::Packfile* packfile : g_packfiles) {
        delete[] packfile->file_list;
        delete packfile;
    }
    g_packfiles.clear();
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

void ForceFileFromPackfile(const char* name, const char* packfile_name)
{
    rf::Packfile* packfile = rf::PackfileFindArchive(packfile_name);
    if (packfile) {
        for (unsigned i = 0; i < packfile->num_files; ++i) {
            if (!stricmp(packfile->file_list[i].file_name, name))
                rf::PackfileAddToLookupTable(&packfile->file_list[i]);
        }
    }
}

void PackfileFindMatchingFiles(const StringMatcher& query, std::function<void(const char*)> result_consumer)
{
    for (int i = 0; i < LOOKUP_TABLE_SIZE; ++i) {
        PackfileLookupTableNew* lookup_table_item = &g_packfile_lookup_table[i];
        if (!lookup_table_item->packfile_entry)
            continue;

        do {
            const char* filename = lookup_table_item->packfile_entry->file_name;
            if (query(filename))
                result_consumer(lookup_table_item->packfile_entry->file_name);
            lookup_table_item = lookup_table_item->next;
        } while (lookup_table_item);
    }
}
