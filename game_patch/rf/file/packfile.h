#pragma once

#include <cstdint>
#include <vector>

namespace rf
{
    struct VPackfileEntry;

    struct VPackfile
    {
        char filename[32];
        char path[128];
        uint32_t field_a0;
        uint32_t num_files;
#ifdef DASH_FACTION
        std::vector<VPackfileEntry> files;
#else
        VPackfileEntry *files;
#endif
        uint32_t file_size;
#ifdef DASH_FACTION
        // track whether the packfile was from user_maps or client_mods
        bool is_user_maps;
        bool is_client_mods;
#endif
    };
#ifndef DASH_FACTION
    static_assert(sizeof(VPackfile) == 0xB0);
#endif

    // Note: this struct memory layout cannot be changed because it is used internally by rf::File class
    struct VPackfileEntry
    {
        uint32_t name_checksum;
        const char* name;
        uint32_t block;
        uint32_t size;
        VPackfile* parent;
        std::FILE* raw_file;
    };
    static_assert(sizeof(VPackfileEntry) == 0x18);

    static auto& vpackfile_add = addr_as_ref<int(const char *file_name, const char *dir)>(0x0052C070);
    static auto& vpackfile_set_loading_user_maps = addr_as_ref<void(bool loading_user_maps)>(0x0052BB50);

    static auto& vpackfile_calc_file_name_checksum = addr_as_ref<uint32_t(const char* file_name)>(0x0052BE70);
    using VPackfileAddEntries_Type = uint32_t(VPackfile* packfile, const void* buf, unsigned num_files_in_block,
                                              unsigned* num_added);
    static auto& vpackfile_add_entries = addr_as_ref<VPackfileAddEntries_Type>(0x0052BD40);
    // handled directly in vpackfile_add_new
    //static auto& vpackfile_loading_user_maps = addr_as_ref<bool>(0x01BDB21C);
}
