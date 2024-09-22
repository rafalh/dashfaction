#include <windows.h>
#include <common/utils/iterable-utils.h>
#include <common/utils/os-utils.h>
#include <xxhash.h>
#include <common/config/BuildConfig.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include <fstream>
#include <format>
#include <array>
#include <cctype>
#include <cstring>
#include <unordered_map>
#include <shlwapi.h>
#include "vpackfile.h"
#include "../main/main.h"
#include "../misc/misc.h"
#include "../rf/file/file.h"
#include "../rf/file/packfile.h"
#include "../rf/crt.h"
#include "../rf/multi.h"
#include "../rf/os/os.h"
#include "../os/console.h"

#define CHECK_PACKFILE_CHECKSUM 0 // slow (1 second on SSD on first load after boot)

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

const char* mod_file_allow_list[] = {
    "reticle_0.tga",
    "reticle_1.tga",
    "scope_ret_0.tga",
    "scope_ret_1.tga",
    "reticle_rocket_0.tga",
    "reticle_rocket_1.tga",
};

static bool is_mod_file_in_whitelist(const char* Filename)
{
    for (unsigned i = 0; i < std::size(mod_file_allow_list); ++i)
        if (!stricmp(mod_file_allow_list[i], Filename))
            return true;
    return false;
}

#endif // MOD_FILE_WHITELIST

// allow list of table files that can't be used for cheating, but for which modding in clientside mods has utility
// Examples include translation packs (translated strings, endgame, etc.) and custom HUD mods that change coords of HUD elements
constexpr std::array<std::string_view, 7> tbl_mod_allow_list = {
    "strings.tbl",
    "hud.tbl",
    "hud_personas.tbl",
    "personas.tbl",
    "credits.tbl",
    "endgame.tbl",
    "ponr.tbl"
};

static bool is_tbl_file(const char* filename)
{
    // confirm we're working with a tbl file
    if (stricmp(rf::file_get_ext(filename), ".tbl") == 0) {
        return true;
    }
    return false;
}

static bool is_tbl_file_in_allowlist(const char* filename)
{
    // compare the input file against the tbl file allowlist
    return is_tbl_file(filename) && std::ranges::any_of(tbl_mod_allow_list, [filename](std::string_view allowed_tbl) {
               return stricmp(allowed_tbl.data(), filename) == 0;
           });
}

static bool is_tbl_file_a_hud_messages_file(const char* filename)
{
    // check if the input file ends with "_text.tbl"
    if (strlen(filename) >= 9 && stricmp(filename + strlen(filename) - 9, "_text.tbl") == 0) {
        return true;
    }
    return false;
}

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
    std::pair<GameLang, const char*> langs[] = {
        {LANG_GR, "gr"},
        {LANG_FR, "fr"},
        {LANG_EN, "en"},
    };

    for (auto& p : langs) {
        auto [lang_id, lang_code] = p;
        auto full_path = std::format("{}maps_{}.vpp", rf::root_path, lang_code);
        BOOL exists = PathFileExistsA(full_path.c_str());
        xlog::info("Checking file {}: {}", full_path, exists ? "found" : "not found");
        if (exists) {
            xlog::info("Detected game language: {}", lang_code);
            return lang_id;
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

static uint32_t vpackfile_process_header(rf::VPackfile* packfile, const void* raw_header)
{
    struct VppHeader
    {
        uint32_t sig;
        uint32_t unk;
        uint32_t num_files;
        uint32_t total_size;
    };
    constexpr unsigned VPP_SIG = 0x51890ACE;
    const auto& hdr = *static_cast<const VppHeader*>(raw_header);
    packfile->num_files = hdr.num_files;
    packfile->file_size = hdr.total_size;
    if (hdr.sig != VPP_SIG || hdr.unk < 1u)
        return 0;
    return hdr.num_files;
}

static int vpackfile_add_new(const char* filename, const char* dir)
{
    xlog::trace("Load packfile {} {}", dir, filename);

    std::string full_path;
    if (dir && !PathIsRelativeA(dir))
        full_path = std::format("{}{}", dir, filename); // absolute path
    else
        full_path = std::format("{}{}{}", rf::root_path, dir ? dir : "", filename);

    if (!filename || strlen(filename) > 0x1F || full_path.size() > 0x7F) {
        xlog::error("Packfile name or path too long: {}", full_path);
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
                xlog::info("VPackfile {} has invalid checksum 0x{:x}", Filename, Checksum);
                g_is_modded_game = true;
            }
        }
    }
#endif // CHECK_PACKFILE_CHECKSUM

    std::ifstream file(full_path, std::ios_base::in | std::ios_base::binary);
    if (!file) {
        xlog::error("Failed to open packfile {}", full_path);
        return 0;
    }
    xlog::info("Opened packfile {}", full_path);

    auto packfile = std::make_unique<rf::VPackfile>();
    std::strncpy(packfile->filename, filename, sizeof(packfile->filename) - 1);
    packfile->filename[sizeof(packfile->filename) - 1] = '\0';
    std::strncpy(packfile->path, full_path.c_str(), sizeof(packfile->path) - 1);
    packfile->path[sizeof(packfile->path) - 1] = '\0';
    packfile->field_a0 = 0;
    packfile->num_files = 0;
    // packfile->is_user_maps = rf::vpackfile_loading_user_maps; // this is set to true for user_maps
    // check for is_user_maps based on dir (like is_client_mods) instead of 0x01BDB21C
    packfile->is_user_maps = (dir && (stricmp(dir, "user_maps\\multi\\") == 0 || stricmp(dir, "user_maps\\single\\") == 0));
    packfile->is_client_mods = (dir && stricmp(dir, "client_mods\\") == 0);

    xlog::debug("Packfile {} is from {}user_maps, {}client_mods", filename, packfile->is_user_maps ? "" : "NOT ",
               packfile->is_client_mods ? "" : "NOT ");

    // Process file header
    char buf[0x800];
    if (!file.read(buf, sizeof(buf))) {
        xlog::error("Failed to read VPP header: {}", filename);
        return 0;
    }
    // Note: VPackfileProcessHeader returns number of files in packfile - result 0 is not always a true error
    if (!vpackfile_process_header(packfile.get(), buf)) {
        return 0;
    }

    // Load all entries
    unsigned current_block = 1;
    packfile->files.resize(packfile->num_files);
    unsigned num_added = 0;
    for (unsigned i = 0; i < packfile->num_files; i += 32) {
        if (!file.read(buf, sizeof(buf))) {
            xlog::error("Failed to read vpp {}", full_path);
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
        if (string_equals_ignore_case(packfile->filename, filename))
            return packfile.get();
    }

    xlog::error("VPackfile {} not found", filename);
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
    filenames = static_cast<char*>(rf::rf_malloc(num_bytes));
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
    // dirs are loaded in this order: base game, user_maps, client_mods, mods
    if (g_is_overriding_disabled) {
        // Don't allow overriding files after game is initialized because it can lead to crashes
        return false;
    }
    // Allow overriding by packfiles from game root and from mods (not user_maps or client_mods)
    else if (!new_entry->parent->is_user_maps && !new_entry->parent->is_client_mods) {
        return true;
    }
#ifdef MOD_FILE_WHITELIST
    else if (is_mod_file_in_whitelist(new_entry->name)) {
        // Always allow overriding for specific files
        return true;
    }
#endif
    // Process if in client_mods
    else if (new_entry->parent->is_client_mods) {
        if (is_tbl_file_in_allowlist(new_entry->name) || is_tbl_file_a_hud_messages_file(new_entry->name)) {
            // allow overriding of tbl files from client_mods if they are on allow list or are _text.tbl
            g_is_modded_game = true; // goober todo: confirm what this is used for. If anticheat, can remove from here
            return true;
        }
        else if (is_tbl_file(new_entry->name)) {
            // deny all other tbl overrides
            return false;
        }
        g_is_modded_game = true;
        return true;
    }
    // Process if in user_maps
    else if (new_entry->parent->is_user_maps) {
        if (old_entry->parent->is_user_maps) {
            // allow files from user_maps to override other files in user_maps, even if the switch is off
            // files from user_maps can't override files from client_mods because client_mods is parsed later
            return true;
        }
        else if (!g_game_config.allow_overwrite_game_files)
        {
            // if the switch is off, no overwriting with files from user_maps, except for other files in user_maps
            return false;
        }
        else if (is_tbl_file_in_allowlist(new_entry->name) || is_tbl_file_a_hud_messages_file(new_entry->name))
        {
            // allow overriding of tbl files from user_maps if they are on allow list or are _text.tbl
            g_is_modded_game = true; // goober todo: confirm what this is used for. If anticheat, can remove from here
            return true;
        }
        else if (is_tbl_file(new_entry->name))
        {
            // deny all other tbl overrides
            return false;
        }
        g_is_modded_game = true;
        return true;
    }
    // resolve warning by having a default option, even though there should be no way to hit it
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
            xlog::trace("Allowed overriding packfile file {} (old packfile {}, new packfile {})", entry->name,
                it->second->parent->filename, entry->parent->filename);
            it->second = entry;
        }
        else {
            xlog::trace("Denied overriding packfile file {} (old packfile {}, new packfile {})", entry->name,
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
    const auto* record = static_cast<const VppFileInfo*>(block);

    for (unsigned i = 0; i < num_files; ++i) {
        const char* file_name = record->name;
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
    xlog::trace("Cannot find file {}", filename_str);
    return nullptr;
}

CodeInjection vpackfile_open_check_seek_result_injection{
    0x0052C301,
    [](auto& regs) {
        int fseek_result = regs.eax;
        regs.eax = fseek_result == 0 ? 1 : 0;
        regs.eip = 0x0052C306;
    },
};

static void load_dashfaction_vpp()
{
    // Load DashFaction specific packfile
    std::string df_vpp_dir = get_module_dir(g_hmodule);
    const char* df_vpp_base_name = "dashfaction.vpp";
    if (!PathFileExistsA((df_vpp_dir + df_vpp_base_name).c_str())) {
        // Remove trailing slash
        if (df_vpp_dir.back() == '\\') {
            df_vpp_dir.pop_back();
        }
        // Remove the last component from the path leaving the trailing slash
        // This is needed to allow running/debugging from MSVC
        auto pos = df_vpp_dir.rfind('\\');
        if (pos != std::string::npos) {
            df_vpp_dir.resize(pos + 1);
        }
    }
    xlog::info("Loading {} from directory: {}", df_vpp_base_name, df_vpp_dir);
    if (!rf::vpackfile_add(df_vpp_base_name, df_vpp_dir.c_str())) {
        xlog::error("Failed to load {}", df_vpp_base_name);
    }
}

ConsoleCommand2 which_packfile_cmd{
    "which_packfile",
    [](std::string filename) {
        auto* entry = vpackfile_find_new(filename.c_str());
        if (entry) {
            rf::console::print("{}", entry->parent->path);
        }
        else {
            rf::console::print("Cannot find {}!", filename);
        }
    },
    "Prints packfile path that the provided file is included in",
};

void force_file_from_packfile(const char* name, const char* packfile_name)
{
    rf::VPackfile* packfile = vpackfile_find_packfile(packfile_name);
    if (packfile) {
        for (auto& entry : packfile->files) {
            if (string_equals_ignore_case(entry.name, name)) {
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
        if (!rf::is_dedicated_server) {
            rf::vpackfile_add("audiog.vpp", nullptr);
            rf::vpackfile_add("maps_gr.vpp", nullptr);
            rf::vpackfile_add("levels1g.vpp", nullptr);
            rf::vpackfile_add("levels2g.vpp", nullptr);
            rf::vpackfile_add("levels3g.vpp", nullptr);
        }
        rf::vpackfile_add("ltables.vpp", nullptr);
    }
    else if (get_installed_game_lang() == LANG_FR) {
        if (!rf::is_dedicated_server) {
            rf::vpackfile_add("audiof.vpp", nullptr);
            rf::vpackfile_add("maps_fr.vpp", nullptr);
            rf::vpackfile_add("levels1f.vpp", nullptr);
            rf::vpackfile_add("levels2f.vpp", nullptr);
            rf::vpackfile_add("levels3f.vpp", nullptr);
        }
        rf::vpackfile_add("ltables.vpp", nullptr);
    }
    else if (!rf::is_dedicated_server) {
        rf::vpackfile_add("audio.vpp", nullptr);
        rf::vpackfile_add("maps_en.vpp", nullptr);
        rf::vpackfile_add("levels1.vpp", nullptr);
        rf::vpackfile_add("levels2.vpp", nullptr);
        rf::vpackfile_add("levels3.vpp", nullptr);
    }
    rf::vpackfile_add("levelsm.vpp", nullptr);
    // rf::vpackfile_add("levelseb.vpp", nullptr);
    // rf::vpackfile_add("levelsbg.vpp", nullptr);
    if (!rf::is_dedicated_server) {
        rf::vpackfile_add("maps1.vpp", nullptr);
        rf::vpackfile_add("maps2.vpp", nullptr);
        rf::vpackfile_add("maps3.vpp", nullptr);
        rf::vpackfile_add("maps4.vpp", nullptr);
    }
    rf::vpackfile_add("meshes.vpp", nullptr);
    rf::vpackfile_add("motions.vpp", nullptr);
    if (!rf::is_dedicated_server) {
        rf::vpackfile_add("music.vpp", nullptr);
        rf::vpackfile_add("ui.vpp", nullptr);
        load_dashfaction_vpp();
    }
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

    if (!rf::is_dedicated_server) {
        // Allow modded strings.tbl in ui.vpp
        force_file_from_packfile("strings.tbl", "ui.vpp");
    }

    xlog::info("Packfiles initialization took {}ms", GetTickCount() - start_ticks);
    xlog::info("Packfile name collisions: {}", g_num_name_collisions);

    if (g_is_modded_game)
        xlog::info("Modded game detected!");

    // Commands
    which_packfile_cmd.register_cmd();
}

static void vpackfile_cleanup_new()
{
    g_packfiles.clear();
}

static void load_vpp_files_from_directory(const char* files, const char* directory)
{
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind = FindFirstFile(files, &find_file_data);

     if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                // Call vpackfile_add_new function directly
                if (vpackfile_add_new(find_file_data.cFileName, directory) == 0) {
                    xlog::warn("Failed to load additional VPP file: {}", find_file_data.cFileName);
                }
            }
        } while (FindNextFile(hFind, &find_file_data) != 0);
        FindClose(hFind);
    }
    else {
        xlog::info("No VPP files found in {}.", directory);
    }
}

static void load_additional_packfiles_new()
{
    // load VPP files from user_maps\single and user_maps\multi first
    load_vpp_files_from_directory("user_maps\\single\\*.vpp", "user_maps\\single\\");
    load_vpp_files_from_directory("user_maps\\multi\\*.vpp", "user_maps\\multi\\");

    // then load VPP files from client_mods
    load_vpp_files_from_directory("client_mods\\*.vpp", "client_mods\\");

    // lastly load VPP files from the loaded TC mod if applicable
    if (tc_mod_is_loaded()) {
        std::string mod_name = rf::mod_param.get_arg();
        std::string mod_dir = "mods\\" + mod_name + "\\";
        std::string mod_file = mod_dir + "*.vpp";
        xlog::info("Loading packfiles from mod: {}.", mod_name);
        //rf::game_add_path(mod_dir.c_str(), ".vpp");
        load_vpp_files_from_directory(mod_file.c_str(), mod_dir.c_str());
    }
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
    AsmWriter(0x004B15E0).jmp(load_additional_packfiles_new);

    // Don't return success from vpackfile_open if offset points out of file contents
    vpackfile_open_check_seek_result_injection.install();

#ifdef DEBUG
    write_mem<u8>(0x0052BEF0, asm_opcodes::int3); // vpackfile_init_file_list
    write_mem<u8>(0x0052BF50, asm_opcodes::int3); // vpackfile_load_internal
    write_mem<u8>(0x0052C440, asm_opcodes::int3); // vpackfile_find_entry
    write_mem<u8>(0x0052BD10, asm_opcodes::int3); // vpackfile_process_header
    write_mem<u8>(0x0052BEB0, asm_opcodes::int3); // vpackfile_setup_file_offsets
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
