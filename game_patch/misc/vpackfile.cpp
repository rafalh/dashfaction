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

static auto& vpackfile_loading_user_maps = addr_as_ref<bool>(0x01BDB21C);

static auto& vpackfile_calc_file_name_checksum = addr_as_ref<uint32_t(const char* file_name)>(0x0052BE70);
typedef uint32_t VPackfileAddEntries_Type(VPackfile* packfile, const void* buf, unsigned num_files_in_block,
                                         unsigned* num_added);
static auto& vpackfile_add_entries = addr_as_ref<VPackfileAddEntries_Type>(0x0052BD40);
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

static bool is_mod_file_in_whitelist(const char* Filename)
{
    for (unsigned i = 0; i < std::size(ModFileWhitelist); ++i)
        if (!stricmp(ModFileWhitelist[i], Filename))
            return true;
    return false;
}

#endif // MOD_FILE_WHITELIST

#if CHECK_PACKFILE_CHECKSUM

static unsigned hash_file(const char* Filename)
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

static GameLang detect_installed_game_lang()
{
    std::array lang_codes{"en", "gr", "fr"};

    for (unsigned i = 0; i < lang_codes.size(); ++i) {
        auto full_path = string_format("%smaps_%s.vpp", rf::root_path, lang_codes[i]);
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

GameLang get_installed_game_lang()
{
    static bool initialized = false;
    static GameLang installed_game_lang;
    if (!initialized) {
        installed_game_lang = detect_installed_game_lang();
        initialized = true;
    }
    return installed_game_lang;
}

bool is_modded_game()
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

static int vpackfile_add_new(const char* filename, const char* dir)
{
    xlog::trace("Load packfile %s %s", dir, filename);

    std::string full_path;
    if (dir && !PathIsRelativeA(dir))
        full_path = string_format("%s%s", dir, filename); // absolute path
    else
        full_path = string_format("%s%s%s", rf::root_path, dir ? dir : "", filename);

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
            unsigned Checksum = hash_file(FullPath);
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
    packfile->is_user_maps = rf::vpackfile_loading_user_maps;

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
        rf::vpackfile_add_entries(packfile.get(), buf, num_files_in_block, &num_added);
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

static rf::VPackfile* vpackfile_find_packfile(const char* filename)
{
    for (auto& packfile : g_packfiles) {
        if (!stricmp(packfile->filename, filename))
            return packfile.get();
    }

    xlog::error("VPackfile %s not found", filename);
    return nullptr;
}

template<typename F>
static void for_each_packfile_entry(const std::vector<std::string_view>& ext_filter, const char* packfile_filter, F fun)
{
    std::vector<std::string> ext_filter_lower;
    ext_filter_lower.reserve(ext_filter.size());
    std::transform(ext_filter.begin(), ext_filter.end(), std::back_inserter(ext_filter_lower), string_to_lower);
    for (auto& packfile : g_packfiles) {
        if (!packfile_filter || !stricmp(packfile_filter, packfile->filename)) {
            for (auto& entry : packfile->files) {
                const char* ext_ptr = rf::file_get_ext(entry.name);
                if (ext_ptr[0]) {
                    ++ext_ptr;
                }
                std::string ext_lower = string_to_lower(ext_ptr);
                if (iterable_contains(ext_filter_lower, ext_lower)) {
                    fun(entry);
                }
            }
        }
    }
}

static int vpackfile_build_file_list_new(const char* ext_filter, char*& filenames, unsigned& num_files,
                                     const char* packfile_filter)
{
    xlog::trace("PackfileBuildFileList begin");
    auto ext_filter_splitted = string_split(ext_filter, ',');
    // Calculate number of bytes needed by result (zero terminated file names + buffer terminating zero)
    unsigned num_bytes = 1;
    for_each_packfile_entry(ext_filter_splitted, packfile_filter, [&](auto& entry) {
        num_bytes += std::strlen(entry.name) + 1;
    });
    // Allocate result buffer
    filenames = static_cast<char*>(rf::Malloc(num_bytes));
    if (!filenames)
        return 0;
    // Fill result buffer and count matching files
    num_files = 0;
    char* buf_ptr = filenames;
    for_each_packfile_entry(ext_filter_splitted, packfile_filter, [&](auto& entry) {
        strcpy(buf_ptr, entry.name);
        buf_ptr += std::strlen(entry.name) + 1;
        ++num_files;
    });
    // Add terminating zero to the buffer
    buf_ptr[0] = 0;
    xlog::trace("PackfileBuildFileList end");
    return 1;
}

static bool is_lookup_table_entry_override_allowed(rf::VPackfileEntry* old_entry, rf::VPackfileEntry* new_entry)
{
    if (g_is_overriding_disabled) {
        // Don't allow overriding files after game is initialized because it can lead to crashes
        return false;
    }
    if (!new_entry->parent->is_user_maps) {
        // Allow overriding by packfiles from game root and from mods
        return true;
    }
    if (!old_entry->parent->is_user_maps && !stricmp(rf::file_get_ext(new_entry->name), ".tbl")) {
        // Always skip overriding tbl files from game by user_maps
        return false;
    }
#ifdef MOD_FILE_WHITELIST
    if (is_mod_file_in_whitelist(new_entry->file_name)) {
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

static void vpackfile_add_to_lookup_table(rf::VPackfileEntry* entry)
{
    std::string filename_str = string_to_lower(entry->name);
    auto [it, inserted] = g_loopup_table.insert({filename_str, entry});
    if (!inserted) {
        ++g_num_name_collisions;
        if (is_lookup_table_entry_override_allowed(it->second, entry)) {
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

static int vpackfile_add_entries_new(rf::VPackfile* packfile, const void* block, unsigned num_files,
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
        entry.name_checksum = rf::vpackfile_calc_file_name_checksum(entry.name);
        entry.size = record->size;
        entry.parent = packfile;
        entry.raw_file = nullptr;

        ++record;
        ++num_added_files;

        vpackfile_add_to_lookup_table(&entry);
        ++g_num_files_in_packfiles;
    }
    return 1;
}

static rf::VPackfileEntry* vpackfile_find_new(const char* filename)
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

CodeInjection vpackfile_open_check_seek_result_injection{
    0x0052C301,
    [](auto& regs) {
        regs.eax = !regs.eax;
        regs.eip = 0x0052C306;
    },
};

static void load_dashfaction_vpp()
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
    if (!rf::vpackfile_add("dashfaction.vpp", buf))
        xlog::error("Failed to load dashfaction.vpp");
}

void force_file_from_packfile(const char* name, const char* packfile_name)
{
    rf::VPackfile* packfile = vpackfile_find_packfile(packfile_name);
    if (packfile) {
        for (auto& entry : packfile->files) {
            if (!stricmp(entry.name, name)) {
                vpackfile_add_to_lookup_table(&entry);
            }
        }
    }
}

static void vpackfile_init_new()
{
    unsigned start_ticks = GetTickCount();

    g_loopup_table.reserve(10000);

    if (get_installed_game_lang() == LANG_GR) {
        rf::vpackfile_add("audiog.vpp", nullptr);
        rf::vpackfile_add("maps_gr.vpp", nullptr);
        rf::vpackfile_add("levels1g.vpp", nullptr);
        rf::vpackfile_add("levels2g.vpp", nullptr);
        rf::vpackfile_add("levels3g.vpp", nullptr);
        rf::vpackfile_add("ltables.vpp", nullptr);
    }
    else if (get_installed_game_lang() == LANG_FR) {
        rf::vpackfile_add("audiof.vpp", nullptr);
        rf::vpackfile_add("maps_fr.vpp", nullptr);
        rf::vpackfile_add("levels1f.vpp", nullptr);
        rf::vpackfile_add("levels2f.vpp", nullptr);
        rf::vpackfile_add("levels3f.vpp", nullptr);
        rf::vpackfile_add("ltables.vpp", nullptr);
    }
    else {
        rf::vpackfile_add("audio.vpp", nullptr);
        rf::vpackfile_add("maps_en.vpp", nullptr);
        rf::vpackfile_add("levels1.vpp", nullptr);
        rf::vpackfile_add("levels2.vpp", nullptr);
        rf::vpackfile_add("levels3.vpp", nullptr);
    }
    rf::vpackfile_add("levelsm.vpp", nullptr);
    // rf::vpackfile_add("levelseb.vpp", nullptr);
    // rf::vpackfile_add("levelsbg.vpp", nullptr);
    rf::vpackfile_add("maps1.vpp", nullptr);
    rf::vpackfile_add("maps2.vpp", nullptr);
    rf::vpackfile_add("maps3.vpp", nullptr);
    rf::vpackfile_add("maps4.vpp", nullptr);
    rf::vpackfile_add("meshes.vpp", nullptr);
    rf::vpackfile_add("motions.vpp", nullptr);
    rf::vpackfile_add("music.vpp", nullptr);
    rf::vpackfile_add("ui.vpp", nullptr);
    load_dashfaction_vpp();
    rf::vpackfile_add("tables.vpp", nullptr);
    addr_as_ref<int>(0x01BDB218) = 1;          // VPackfilesLoaded
    addr_as_ref<uint32_t>(0x01BDB210) = 10000; // NumFilesInVfs
    addr_as_ref<uint32_t>(0x01BDB214) = 100;   // NumPackfiles

    // Note: language changes in binary are done here to make sure RootPath is already initialized

    // Switch UI language - can be anything even if this is US edition
    auto installation_lang = get_installed_game_lang();
    int lang_id = installation_lang;
    if (g_game_config.language >= 0 && g_game_config.language < 3) {
        lang_id = g_game_config.language;
    }
    write_mem<u8>(0x004B27D2 + 1, static_cast<uint8_t>(lang_id));

    // Switch localized tables names
    if (installation_lang != LANG_EN) {
        write_mem_ptr(0x0043DCAB + 1, "localized_credits.tbl");
        write_mem_ptr(0x0043E50B + 1, "localized_endgame.tbl");
        write_mem_ptr(0x004B082B + 1, "localized_strings.tbl");
    }

    // Allow modded strings.tbl in ui.vpp
    force_file_from_packfile("strings.tbl", "ui.vpp");

    xlog::info("Packfiles initialization took %lums", GetTickCount() - start_ticks);
    xlog::info("VPackfile name collisions: %d", g_num_name_collisions);

    if (g_is_modded_game)
        xlog::info("Modded game detected!");
}

static void vpackfile_cleanup_new()
{
    g_packfiles.clear();
}

void vpackfile_apply_patches()
{
    // VPackfile handling implemetation getting rid of all limits

    AsmWriter(0x0052BD40).jmp(vpackfile_add_entries_new);
    AsmWriter(0x0052C4D0).jmp(vpackfile_build_file_list_new);
    AsmWriter(0x0052C070).jmp(vpackfile_add_new);
    AsmWriter(0x0052C220).jmp(vpackfile_find_new);
    AsmWriter(0x0052BB60).jmp(vpackfile_init_new);
    AsmWriter(0x0052BC80).jmp(vpackfile_cleanup_new);

    // Don't return success from VPackfileOpen if offset points out of file contents
    vpackfile_open_check_seek_result_injection.install();

#ifdef DEBUG
    write_mem<u8>(0x0052BEF0, asm_opcodes::int3); // VPackfileInitFileList
    write_mem<u8>(0x0052BF50, asm_opcodes::int3); // VPackfileLoadInternal
    write_mem<u8>(0x0052C440, asm_opcodes::int3); // VPackfileFindEntry
    write_mem<u8>(0x0052BD10, asm_opcodes::int3); // VPackfileProcessHeader
    write_mem<u8>(0x0052BEB0, asm_opcodes::int3); // VPackfileSetupFileOffsets
    write_mem<u8>(0x0052BCA0, asm_opcodes::int3); // vpackfile_add_to_lookup_table
    write_mem<u8>(0x0052C1D0, asm_opcodes::int3); // vpackfile_find_packfile

#endif
}

void vpackfile_find_matching_files(const StringMatcher& query, std::function<void(const char*)> result_consumer)
{
    for (auto& p : g_loopup_table) {
        if (query(p.first.c_str())) {
            result_consumer(p.first.c_str());
        }
    }
}

void vpackfile_disable_overriding()
{
    g_is_overriding_disabled = true;
}
