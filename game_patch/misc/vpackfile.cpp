#include <windows.h>
#include "vpackfile.h"
#include "../main.h"
#include "../rf/file.h"
#include "../utils/iterable-utils.h"
#include <xxhash.h>
#include <common/BuildConfig.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include <fstream>
#include <array>
#include <cctype>
#include <cstring>
#include <unordered_map>
#include <shlwapi.h>

#define CHECK_PACKFILE_CHECKSUM 0 // slow (1 second on SSD on first load after boot)

namespace rf
{
struct VPackfileEntry;

struct VPackfile
{
    char filename[32];
    char path[128];
    uint32_t field_a0;
    uint32_t num_files;
    std::vector<VPackfileEntry> files;
    uint32_t file_size;
    bool is_user_maps;
};

// Note: this struct memory layout cannot be changed because it is used internally by rf::File class
struct VPackfileEntry
{
    uint32_t name_checksum;
    const char* name;
    uint32_t block;
    uint32_t size;
    VPackfile* parent;
    FILE* raw_file;
};

static auto& packfile_loading_user_maps = AddrAsRef<bool>(0x01BDB21C);

static auto& VPackfileCalcFileNameChecksum = AddrAsRef<uint32_t(const char* file_name)>(0x0052BE70);
typedef uint32_t VPackfileAddEntries_Type(VPackfile* packfile, const void* buf, unsigned num_files_in_block,
                                         unsigned* num_added);
static auto& VPackfileAddEntries = AddrAsRef<VPackfileAddEntries_Type>(0x0052BD40);
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
static std::vector<std::unique_ptr<rf::VPackfile>> g_packfiles;
static std::unordered_map<std::string, rf::VPackfileEntry*> g_loopup_table;
static bool g_is_modded_game = false;
static bool g_is_overriding_disabled = false;

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

static uint32_t VPackfileProcessHeader(rf::VPackfile* packfile, const void* raw_header)
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
    packfile->file_size = hdr.total_size;
    if (hdr.sig != 0x51890ACE || hdr.unk < 1u)
        return 0;
    return hdr.num_files;
}

static int VPackfileAdd_New(const char* filename, const char* dir)
{
    xlog::trace("Load packfile %s %s", dir, filename);

    std::string full_path;
    if (dir && !PathIsRelativeA(dir))
        full_path = StringFormat("%s%s", dir, filename); // absolute path
    else
        full_path = StringFormat("%s%s%s", rf::root_path, dir ? dir : "", filename);

    if (!filename || strlen(filename) > 0x1F || full_path.size() > 0x7F) {
        xlog::error("VPackfile name or path too long: %s", full_path.c_str());
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
                xlog::info("VPackfile %s has invalid checksum 0x%X", Filename, Checksum);
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

    auto packfile = std::make_unique<rf::VPackfile>();
    std::strncpy(packfile->filename, filename, sizeof(packfile->filename) - 1);
    packfile->filename[sizeof(packfile->filename) - 1] = '\0';
    std::strncpy(packfile->path, full_path.c_str(), sizeof(packfile->path) - 1);
    packfile->path[sizeof(packfile->path) - 1] = '\0';
    packfile->field_a0 = 0;
    packfile->num_files = 0;
    // this is set to true for user_maps
    packfile->is_user_maps = rf::packfile_loading_user_maps;

    // Process file header
    char buf[0x800];
    if (!file.read(buf, sizeof(buf))) {
        xlog::error("Failed to read VPP header: %s", filename);
        return 0;
    }
    // Note: VPackfileProcessHeader returns number of files in packfile - result 0 is not always a true error
    if (!VPackfileProcessHeader(packfile.get(), buf)) {
        return 0;
    }

    // Load all entries
    unsigned current_block = 1;
    packfile->files.resize(packfile->num_files);
    unsigned num_added = 0;
    for (unsigned i = 0; i < packfile->num_files; i += 32) {
        if (!file.read(buf, sizeof(buf))) {
            xlog::error("Failed to read vpp %s", full_path.c_str());
            return 0;
        }

        unsigned num_files_in_block = std::min(packfile->num_files - i, 32u);
        rf::VPackfileAddEntries(packfile.get(), buf, num_files_in_block, &num_added);
        ++current_block;
    }
    packfile->files.resize(num_added);

    // Set block in all entries
    for (auto& entry : packfile->files) {
        entry.block = current_block;
        current_block += (entry.size + 2047) / 2048;
    }

    g_packfiles.push_back(std::move(packfile));

    return 1;
}

static rf::VPackfile* VPackfileFindPackfile(const char* filename)
{
    for (auto& packfile : g_packfiles) {
        if (!stricmp(packfile->filename, filename))
            return packfile.get();
    }

    xlog::error("VPackfile %s not found", filename);
    return nullptr;
}

template<typename F>
static void ForEachPackfileEntry(const std::vector<std::string_view>& ext_filter, const char* packfile_filter, F fun)
{
    std::vector<std::string> ext_filter_lower;
    ext_filter_lower.reserve(ext_filter.size());
    std::transform(ext_filter.begin(), ext_filter.end(), std::back_inserter(ext_filter_lower), StringToLower);
    for (auto& packfile : g_packfiles) {
        if (!packfile_filter || !stricmp(packfile_filter, packfile->filename)) {
            for (auto& entry : packfile->files) {
                const char* ext_ptr = rf::FileGetExt(entry.name);
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

static int VPackfileBuildFileList_New(const char* ext_filter, char*& filenames, unsigned& num_files,
                                     const char* packfile_filter)
{
    xlog::trace("PackfileBuildFileList begin");
    auto ext_filter_splitted = StringSplit(ext_filter, ',');
    // Calculate number of bytes needed by result (zero terminated file names + buffer terminating zero)
    unsigned num_bytes = 1;
    ForEachPackfileEntry(ext_filter_splitted, packfile_filter, [&](auto& entry) {
        num_bytes += std::strlen(entry.name) + 1;
    });
    // Allocate result buffer
    filenames = static_cast<char*>(rf::Malloc(num_bytes));
    if (!filenames)
        return 0;
    // Fill result buffer and count matching files
    num_files = 0;
    char* buf_ptr = filenames;
    ForEachPackfileEntry(ext_filter_splitted, packfile_filter, [&](auto& entry) {
        strcpy(buf_ptr, entry.name);
        buf_ptr += std::strlen(entry.name) + 1;
        ++num_files;
    });
    // Add terminating zero to the buffer
    buf_ptr[0] = 0;
    xlog::trace("PackfileBuildFileList end");
    return 1;
}

static bool IsLookupTableEntryOverrideAllowed(rf::VPackfileEntry* old_entry, rf::VPackfileEntry* new_entry)
{
    if (g_is_overriding_disabled) {
        // Don't allow overriding files after game is initialized because it can lead to crashes
        return false;
    }
    if (!new_entry->parent->is_user_maps) {
        // Allow overriding by packfiles from game root and from mods
        return true;
    }
    if (!old_entry->parent->is_user_maps && !stricmp(rf::FileGetExt(new_entry->name), ".tbl")) {
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

static void VPackfileAddToLookupTable(rf::VPackfileEntry* entry)
{
    std::string filename_str = StringToLower(entry->name);
    auto [it, inserted] = g_loopup_table.insert({filename_str, entry});
    if (!inserted) {
        ++g_num_name_collisions;
        if (IsLookupTableEntryOverrideAllowed(it->second, entry)) {
            xlog::trace("Allowed overriding packfile file %s (old packfile %s, new packfile %s)", entry->name,
                it->second->parent->filename, entry->parent->filename);
            it->second = entry;
        }
        else {
            xlog::trace("Denied overriding packfile file %s (old packfile %s, new packfile %s)", entry->name,
                it->second->parent->filename, entry->parent->filename);
        }
    }
}

static int VPackfileAddEntries_New(rf::VPackfile* packfile, const void* block, unsigned num_files,
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
        rf::VPackfileEntry& entry = packfile->files[num_added_files];

        // Note: we can't use string pool from RF because it's too small
        char* file_name_buf = new char[strlen(file_name) + 1];
        std::strcpy(file_name_buf, file_name);
        entry.name = file_name_buf;
        entry.name_checksum = rf::VPackfileCalcFileNameChecksum(entry.name);
        entry.size = record->size;
        entry.parent = packfile;
        entry.raw_file = nullptr;

        ++record;
        ++num_added_files;

        VPackfileAddToLookupTable(&entry);
        ++g_num_files_in_packfiles;
    }
    return 1;
}

static rf::VPackfileEntry* VPackfileFind_New(const char* filename)
{
    std::string filename_str{filename};
    std::transform(filename_str.begin(), filename_str.end(), filename_str.begin(), [](unsigned char ch) {
        return std::tolower(ch);
    });
    auto it = g_loopup_table.find(filename_str);
    if (it != g_loopup_table.end()) {
        return it->second;
    }
    xlog::trace("Cannot find file %s", filename_str.c_str());
    return nullptr;
}

CodeInjection VPackfileOpen_check_seek_result_injection{
    0x0052C301,
    [](auto& regs) {
        regs.eax = !regs.eax;
        regs.eip = 0x0052C306;
    },
};

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
    if (!rf::VPackfileAdd("dashfaction.vpp", buf))
        xlog::error("Failed to load dashfaction.vpp");
}

void ForceFileFromPackfile(const char* name, const char* packfile_name)
{
    rf::VPackfile* packfile = VPackfileFindPackfile(packfile_name);
    if (packfile) {
        for (auto& entry : packfile->files) {
            if (!stricmp(entry.name, name)) {
                VPackfileAddToLookupTable(&entry);
            }
        }
    }
}

static void VPackfileInit_New()
{
    unsigned start_ticks = GetTickCount();

    g_loopup_table.reserve(10000);

    if (GetInstalledGameLang() == LANG_GR) {
        rf::VPackfileAdd("audiog.vpp", nullptr);
        rf::VPackfileAdd("maps_gr.vpp", nullptr);
        rf::VPackfileAdd("levels1g.vpp", nullptr);
        rf::VPackfileAdd("levels2g.vpp", nullptr);
        rf::VPackfileAdd("levels3g.vpp", nullptr);
        rf::VPackfileAdd("ltables.vpp", nullptr);
    }
    else if (GetInstalledGameLang() == LANG_FR) {
        rf::VPackfileAdd("audiof.vpp", nullptr);
        rf::VPackfileAdd("maps_fr.vpp", nullptr);
        rf::VPackfileAdd("levels1f.vpp", nullptr);
        rf::VPackfileAdd("levels2f.vpp", nullptr);
        rf::VPackfileAdd("levels3f.vpp", nullptr);
        rf::VPackfileAdd("ltables.vpp", nullptr);
    }
    else {
        rf::VPackfileAdd("audio.vpp", nullptr);
        rf::VPackfileAdd("maps_en.vpp", nullptr);
        rf::VPackfileAdd("levels1.vpp", nullptr);
        rf::VPackfileAdd("levels2.vpp", nullptr);
        rf::VPackfileAdd("levels3.vpp", nullptr);
    }
    rf::VPackfileAdd("levelsm.vpp", nullptr);
    // rf::VPackfileAdd("levelseb.vpp", nullptr);
    // rf::VPackfileAdd("levelsbg.vpp", nullptr);
    rf::VPackfileAdd("maps1.vpp", nullptr);
    rf::VPackfileAdd("maps2.vpp", nullptr);
    rf::VPackfileAdd("maps3.vpp", nullptr);
    rf::VPackfileAdd("maps4.vpp", nullptr);
    rf::VPackfileAdd("meshes.vpp", nullptr);
    rf::VPackfileAdd("motions.vpp", nullptr);
    rf::VPackfileAdd("music.vpp", nullptr);
    rf::VPackfileAdd("ui.vpp", nullptr);
    LoadDashFactionVpp();
    rf::VPackfileAdd("tables.vpp", nullptr);
    AddrAsRef<int>(0x01BDB218) = 1;          // VPackfilesLoaded
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
    xlog::info("VPackfile name collisions: %d", g_num_name_collisions);

    if (g_is_modded_game)
        xlog::info("Modded game detected!");
}

static void VPackfileCleanup_New()
{
    g_packfiles.clear();
}

void VPackfileApplyPatches()
{
    // VPackfile handling implemetation getting rid of all limits

    AsmWriter(0x0052BD40).jmp(VPackfileAddEntries_New);
    AsmWriter(0x0052C4D0).jmp(VPackfileBuildFileList_New);
    AsmWriter(0x0052C070).jmp(VPackfileAdd_New);
    AsmWriter(0x0052C220).jmp(VPackfileFind_New);
    AsmWriter(0x0052BB60).jmp(VPackfileInit_New);
    AsmWriter(0x0052BC80).jmp(VPackfileCleanup_New);

    // Don't return success from VPackfileOpen if offset points out of file contents
    VPackfileOpen_check_seek_result_injection.Install();

#ifdef DEBUG
    WriteMem<u8>(0x0052BEF0, asm_opcodes::int3); // VPackfileInitFileList
    WriteMem<u8>(0x0052BF50, asm_opcodes::int3); // VPackfileLoadInternal
    WriteMem<u8>(0x0052C440, asm_opcodes::int3); // VPackfileFindEntry
    WriteMem<u8>(0x0052BD10, asm_opcodes::int3); // VPackfileProcessHeader
    WriteMem<u8>(0x0052BEB0, asm_opcodes::int3); // VPackfileSetupFileOffsets
    WriteMem<u8>(0x0052BCA0, asm_opcodes::int3); // VPackfileAddToLookupTable
    WriteMem<u8>(0x0052C1D0, asm_opcodes::int3); // VPackfileFindPackfile

#endif
}

void VPackfileFindMatchingFiles(const StringMatcher& query, std::function<void(const char*)> result_consumer)
{
    for (auto& p : g_loopup_table) {
        if (query(p.first.c_str())) {
            result_consumer(p.first.c_str());
        }
    }
}

void VPackfileDisableOverriding()
{
    g_is_overriding_disabled = true;
}
