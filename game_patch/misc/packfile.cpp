#include "packfile.h"
#include "../main.h"
#include "../rf/misc.h"
#include "../utils/iterable-utils.h"
#include <xxhash.h>
#include <common/BuildConfig.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <fstream>
#include <array>
#include <cctype>
#include <unordered_map>
#include <shlwapi.h>

#define DEBUG_VFS 0
#define DEBUG_VFS_FILENAME1 "DM RTS MiniGolf 2.1.rfl"
#define DEBUG_VFS_FILENAME2 "JumpPad.wav"

namespace rf
{
struct PackfileEntry;

struct Packfile
{
    char name[32];
    char path[128];
    uint32_t field_a0;
    uint32_t num_files;
    std::vector<PackfileEntry> files;
    uint32_t packfile_size;
    bool is_user_maps;
};

// Note: this struct memory layout cannot be changed because it is used internally by rf::File class
struct PackfileEntry
{
    uint32_t name_checksum;
    const char* file_name;
    uint32_t offset_in_blocks;
    uint32_t file_size;
    Packfile* archive;
    FILE* raw_file;
};

static auto& packfile_ignore_tbl_files = AddrAsRef<bool>(0x01BDB21C);

static auto& PackfileCalcFileNameChecksum = AddrAsRef<uint32_t(const char* file_name)>(0x0052BE70);
typedef uint32_t PackfileAddEntries_Type(Packfile* packfile, const void* buf, unsigned num_files_in_block,
                                         unsigned* num_added);
static auto& PackfileAddEntries = AddrAsRef<PackfileAddEntries_Type>(0x0052BD40);
} // namespace rf

#if CHECK_PACKFILE_CHECKSUM

const std::map<std::string, unsigned> GameFileChecksums = {
    // Note: Multiplayer level checksum is checked when loading level
    //{ "levelsm.vpp", 0x17D0D38A },
    // Note: maps are big and slow downs startup
    {"maps1.vpp", 0x52EE4F99},  {"maps2.vpp", 0xB053486F},   {"maps3.vpp", 0xA5ED6271},  {"maps4.vpp", 0xE0AB4397},
    {"meshes.vpp", 0xEBA19172}, {"motions.vpp", 0x17132D8E}, {"tables.vpp", 0x549DAABF},
};

#endif // CHECK_PACKFILE_CHECKSUM

static unsigned g_num_files_in_packfiles = 0;
static unsigned g_num_name_collisions = 0;
static std::vector<std::unique_ptr<rf::Packfile>> g_packfiles;
static std::unordered_map<std::string, rf::PackfileEntry*> g_loopup_table;
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
        xlog::info("Checking file %s: %s", full_path.c_str(), exists ? "found" : "not found");
        if (exists) {
            xlog::info("Detected game language: %s", lang_codes[i]);
            return static_cast<GameLang>(i);
        }
    }
    xlog::warn("Cannot detect game language");
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

static uint32_t PackfileProcessHeader(rf::Packfile* packfile, const void* raw_header)
{
    struct VppHeader
    {
        uint32_t sig;
        uint32_t unk;
        uint32_t num_files;
        uint32_t total_size;
    };
    auto& hdr = *reinterpret_cast<const VppHeader*>(raw_header);
    packfile->num_files = hdr.num_files;
    packfile->packfile_size = hdr.total_size;
    if (hdr.sig != 0x51890ACE || hdr.unk < 1u)
        return 0;
    return hdr.num_files;
}

static int PackfileLoad_New(const char* filename, const char* dir)
{
    xlog::trace("Load packfile %s %s", dir, filename);

    std::string full_path;
    if (dir && dir[0] && dir[1] == ':')
        full_path = StringFormat("%s%s", dir, filename); // absolute path
    else
        full_path = StringFormat("%s%s%s", rf::root_path, dir ? dir : "", filename);

    if (!filename || strlen(filename) > 0x1F || full_path.size() > 0x7F) {
        xlog::error("Packfile name or path too long: %s", full_path.c_str());
        return 0;
    }

    for (auto& packfile : g_packfiles)
        if (!stricmp(packfile->path, full_path.c_str()))
            return 1;

#if CHECK_PACKFILE_CHECKSUM
    if (!Dir) {
        auto it = GameFileChecksums.find(Filename);
        if (it != GameFileChecksums.end()) {
            unsigned Checksum = HashFile(FullPath);
            if (Checksum != it->second) {
                xlog::info("Packfile %s has invalid checksum 0x%X", Filename, Checksum);
                g_is_modded_game = true;
            }
        }
    }
#endif // CHECK_PACKFILE_CHECKSUM

    std::ifstream file(full_path.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!file) {
        xlog::error("Failed to open packfile %s", full_path.c_str());
        return 0;
    }

    auto packfile = std::make_unique<rf::Packfile>();
    std::strncpy(packfile->name, filename, sizeof(packfile->name) - 1);
    packfile->name[sizeof(packfile->name) - 1] = '\0';
    std::strncpy(packfile->path, full_path.c_str(), sizeof(packfile->path) - 1);
    packfile->path[sizeof(packfile->path) - 1] = '\0';
    packfile->field_a0 = 0;
    packfile->num_files = 0;
    // this is set to true for user_maps
    packfile->is_user_maps = rf::packfile_ignore_tbl_files;

    // Process file header
    char buf[0x800];
    if (!file.read(buf, sizeof(buf))) {
        xlog::error("Failed to read VPP header: %s", filename);
        return 0;
    }
    // Note: PackfileProcessHeader returns number of files in packfile - result 0 is not always a true error
    if (!PackfileProcessHeader(packfile.get(), buf)) {
        return 0;
    }

    // Load all entries
    unsigned offset_in_blocks = 1;
    packfile->files.resize(packfile->num_files);
    unsigned num_added = 0;
    for (unsigned i = 0; i < packfile->num_files; i += 32) {
        if (!file.read(buf, sizeof(buf))) {
            xlog::error("Failed to read vpp %s", full_path.c_str());
            return 0;
        }

        unsigned num_files_in_block = std::min(packfile->num_files - i, 32u);
        rf::PackfileAddEntries(packfile.get(), buf, num_files_in_block, &num_added);
        ++offset_in_blocks;
    }
    packfile->files.resize(num_added);

    // Set offset_in_blocks in all entries
    for (auto& entry : packfile->files) {
        entry.offset_in_blocks = offset_in_blocks;
        offset_in_blocks += (entry.file_size + 2047) / 2048;
    }

    g_packfiles.push_back(std::move(packfile));

    return 1;
}

static rf::Packfile* PackfileFindArchive(const char* filename)
{
    for (auto& packfile : g_packfiles) {
        if (!stricmp(packfile->name, filename))
            return packfile.get();
    }

    xlog::error("Packfile %s not found", filename);
    return nullptr;
}

template<typename F>
static void ForEachPackfileEntry(const std::vector<std::string_view>& ext_filter, const char* packfile_filter, F fun)
{
    std::vector<std::string> ext_filter_lower;
    ext_filter_lower.reserve(ext_filter.size());
    std::transform(ext_filter.begin(), ext_filter.end(), std::back_inserter(ext_filter_lower), StringToLower);
    for (auto& packfile : g_packfiles) {
        if (!packfile_filter || !stricmp(packfile_filter, packfile->name)) {
            for (auto& entry : packfile->files) {
                const char* ext_ptr = rf::GetFileExt(entry.file_name);
                if (ext_ptr[0]) {
                    ++ext_ptr;
                }
                std::string ext_lower = StringToLower(ext_ptr);
                if (IterableContains(ext_filter_lower, ext_lower)) {
                    fun(entry);
                }
            }
        }
    }
}

static int PackfileBuildFileList_New(const char* ext_filter, char*& filenames, unsigned& num_files,
                                     const char* packfile_filter)
{
    xlog::trace("PackfileBuildFileList begin");
    auto ext_filter_splitted = StringSplit(ext_filter, ",");
    // Calculate number of bytes needed by result (zero terminated file names + buffer terminating zero)
    unsigned num_bytes = 1;
    ForEachPackfileEntry(ext_filter_splitted, packfile_filter, [&](auto& entry) {
        num_bytes += std::strlen(entry.file_name) + 1;
    });
    // Allocate result buffer
    filenames = static_cast<char*>(rf::Malloc(num_bytes));
    if (!filenames)
        return 0;
    // Fill result buffer and count matching files
    num_files = 0;
    char* buf_ptr = filenames;
    ForEachPackfileEntry(ext_filter_splitted, packfile_filter, [&](auto& entry) {
        strcpy(buf_ptr, entry.file_name);
        buf_ptr += std::strlen(entry.file_name) + 1;
        ++num_files;
    });
    // Add terminating zero to the buffer
    buf_ptr[0] = 0;
    xlog::trace("PackfileBuildFileList end");
    return 1;
}

static bool IsLookupTableEntryOverrideAllowed(rf::PackfileEntry* old_entry, rf::PackfileEntry* new_entry)
{
    if (!new_entry->archive->is_user_maps) {
        // Allow overriding by packfiles from game root and from mods
        return true;
    }
    if (!old_entry->archive->is_user_maps && !stricmp(rf::GetFileExt(new_entry->file_name), ".tbl")) {
        // Always skip overriding tbl files from game by user_maps
        return false;
    }
#ifdef MOD_FILE_WHITELIST
    if (IsModFileInWhitelist(new_entry->file_name)) {
        // Always allow overriding for specific files
        return true;
    }
#endif
    if (!g_game_config.allow_overwrite_game_files) {
        return false;
    }
    g_is_modded_game = true;
    return true;
}

static void PackfileAddToLookupTable(rf::PackfileEntry* entry)
{
    std::string filename_str = StringToLower(entry->file_name);
    auto [it, inserted] = g_loopup_table.insert({filename_str, entry});
    if (!inserted) {
        ++g_num_name_collisions;
        if (IsLookupTableEntryOverrideAllowed(it->second, entry)) {
            xlog::trace("Allowed overriding packfile file %s (old packfile %s, new packfile %s)", entry->file_name,
                it->second->archive->name, entry->archive->name);
            it->second = entry;
        }
        else {
            xlog::trace("Denied overriding packfile file %s (old packfile %s, new packfile %s)", entry->file_name,
                it->second->archive->name, entry->archive->name);
        }
    }
}

static int PackfileAddEntries_New(rf::Packfile* packfile, const void* block, unsigned num_files,
                                  unsigned& num_added_files)
{
    struct VppFileInfo
    {
        char name[60];
        uint32_t size;
    };
    auto record = reinterpret_cast<const VppFileInfo*>(block);

    for (unsigned i = 0; i < num_files; ++i) {
        auto file_name = record->name;
        rf::PackfileEntry& entry = packfile->files[num_added_files];

        // Note: we can't use string pool from RF because it's too small
        char* file_name_buf = new char[strlen(file_name) + 1];
        std::strcpy(file_name_buf, file_name);
        entry.file_name = file_name_buf;
        entry.name_checksum = rf::PackfileCalcFileNameChecksum(entry.file_name);
        entry.file_size = record->size;
        entry.archive = packfile;
        entry.raw_file = nullptr;

        ++record;
        ++num_added_files;

        PackfileAddToLookupTable(&entry);
        ++g_num_files_in_packfiles;
    }
    return 1;
}

static rf::PackfileEntry* PackfileFindFile_New(const char* filename)
{
    std::string filename_str{filename};
    std::transform(filename_str.begin(), filename_str.end(), filename_str.begin(), ::tolower);
    auto it = g_loopup_table.find(filename_str);
    if (it != g_loopup_table.end()) {
        return it->second;
    }
    xlog::trace("Cannot find file %s", filename_str.c_str());
    return nullptr;
}

static void LoadDashFactionVpp()
{
    // Load DashFaction specific packfile
    char buf[MAX_PATH];
    GetModuleFileNameA(g_hmodule, buf, sizeof(buf));
    char* ptr = strrchr(buf, '\\');
    std::strcpy(ptr + 1, "dashfaction.vpp");
    if (PathFileExistsA(buf))
        *(ptr + 1) = '\0';
    else {
        // try to remove 3 path components (build/(debug|release)/bin)
        *ptr = '\0';
        ptr = std::strrchr(buf, '\\');
        if (ptr)
            *ptr = '\0';
        ptr = std::strrchr(buf, '\\');
        if (ptr)
            *ptr = '\0';
        ptr = std::strrchr(buf, '\\');
        if (ptr)
            *(ptr + 1) = '\0';
    }
    xlog::info("Loading dashfaction.vpp from directory: %s", buf);
    if (!rf::PackfileLoad("dashfaction.vpp", buf))
        xlog::error("Failed to load dashfaction.vpp");
}

void ForceFileFromPackfile(const char* name, const char* packfile_name)
{
    rf::Packfile* packfile = PackfileFindArchive(packfile_name);
    if (packfile) {
        for (auto& entry : packfile->files) {
            if (!stricmp(entry.file_name, name)) {
                PackfileAddToLookupTable(&entry);
            }
        }
    }
}

void PrioritizePackfile(rf::Packfile* packfile)
{
    for (auto& entry : packfile->files) {
        PackfileAddToLookupTable(&entry);
    }
}

void PrioritizePackfileWithFile(const char* filename)
{
    auto entry = PackfileFindFile_New(filename);
    if (entry) {
        PrioritizePackfile(entry->archive);
    }
}

static void PackfileInit_New()
{
    unsigned start_ticks = GetTickCount();

    g_loopup_table.reserve(10000);

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
    auto installation_lang = GetInstalledGameLang();
    int lang_id = installation_lang;
    if (g_game_config.language >= 0 && g_game_config.language < 3) {
        lang_id = g_game_config.language;
    }
    WriteMem<u8>(0x004B27D2 + 1, static_cast<uint8_t>(lang_id));

    // Switch localized tables names
    if (installation_lang != LANG_EN) {
        WriteMemPtr(0x0043DCAB + 1, "localized_credits.tbl");
        WriteMemPtr(0x0043E50B + 1, "localized_endgame.tbl");
        WriteMemPtr(0x004B082B + 1, "localized_strings.tbl");
    }

    // Allow modded strings.tbl in ui.vpp
    ForceFileFromPackfile("strings.tbl", "ui.vpp");

    xlog::info("Packfiles initialization took %lums", GetTickCount() - start_ticks);
    xlog::info("Packfile name collisions: %d", g_num_name_collisions);

    if (g_is_modded_game)
        xlog::info("Modded game detected!");
}

static void PackfileCleanup_New()
{
    g_packfiles.clear();
}

void PackfileApplyPatches()
{
    // Packfile handling implemetation getting rid of all limits

    AsmWriter(0x0052BD40).jmp(PackfileAddEntries_New);
    AsmWriter(0x0052C4D0).jmp(PackfileBuildFileList_New);
    AsmWriter(0x0052C070).jmp(PackfileLoad_New);
    AsmWriter(0x0052C220).jmp(PackfileFindFile_New);
    AsmWriter(0x0052BB60).jmp(PackfileInit_New);
    AsmWriter(0x0052BC80).jmp(PackfileCleanup_New);

#ifdef DEBUG
    WriteMem<u8>(0x0052BEF0, asm_opcodes::int3); // PackfileInitFileList
    WriteMem<u8>(0x0052BF50, asm_opcodes::int3); // PackfileLoadInternal
    WriteMem<u8>(0x0052C440, asm_opcodes::int3); // PackfileFindEntry
    WriteMem<u8>(0x0052BD10, asm_opcodes::int3); // PackfileProcessHeader
    WriteMem<u8>(0x0052BEB0, asm_opcodes::int3); // PackfileSetupFileOffsets
    WriteMem<u8>(0x0052BCA0, asm_opcodes::int3); // PackfileAddToLookupTable
    WriteMem<u8>(0x0052C1D0, asm_opcodes::int3); // PackfileFindArchive

#endif
}

void PackfileFindMatchingFiles(const StringMatcher& query, std::function<void(const char*)> result_consumer)
{
    for (auto& p : g_loopup_table) {
        if (query(p.first.c_str())) {
            result_consumer(p.first.c_str());
        }
    }
}
