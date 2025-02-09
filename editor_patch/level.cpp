#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/MemUtils.h>
#include <xlog/xlog.h>
#include "vtypes.h"
#include "mfc_types.h"
#include "resources.h"

constexpr std::size_t level_dash_props_offset = 0x608;
constexpr std::size_t level_dialog_offset = 0x270;
constexpr int dash_level_props_chunk_id = 0xDA58FA00;

struct LevelDashProps
{
    bool lightmaps_full_depth = true;

    void ResetBackwardCompatible()
    {
        lightmaps_full_depth = false;
    }

    void Serialize(rf::File& file) const
    {
        file.write<std::uint8_t>(lightmaps_full_depth);
    }

    void Deserialize(rf::File& file)
    {
        lightmaps_full_depth = file.read<std::uint8_t>();
        xlog::debug("lightmaps_full_depth {}", lightmaps_full_depth);
    }
};

struct CRedLevel
{
    [[nodiscard]] std::size_t BeginRflSection(rf::File& file, int chunk_id)
    {
        return AddrCaller{0x00430B60}.this_call<std::size_t>(this, &file, chunk_id);
    }

    void EndRflSection(rf::File& file, std::size_t start_pos)
    {
        return AddrCaller{0x00430B90}.this_call(this, &file, start_pos);
    }

    LevelDashProps& GetDashProps()
    {
        return struct_field_ref<LevelDashProps>(this, level_dash_props_offset);
    }

    static CRedLevel* Get()
    {
        return AddrCaller{0x004835F0}.c_call<CRedLevel*>();
    }
};

CodeInjection CRedLevel_ct_inj{
    0x004181B8,
    [](auto& regs) {
        std::byte* level = regs.esi;
        new (&level[level_dash_props_offset]) LevelDashProps();
    },
};

CodeInjection CRedLevel_LoadLevel_inj0{
    0x0042F136,
    []() {
        CRedLevel::Get()->GetDashProps().ResetBackwardCompatible();
    },
};

CodeInjection CRedLevel_LoadLevel_inj1{
    0x0042F2D4,
    [](auto& regs) {
        auto& level = *static_cast<CRedLevel*>(regs.ebp);
        auto& file = *static_cast<rf::File*>(regs.esi);
        int chunk_id = regs.edi;
        std::size_t chunk_size = regs.ebx;
        if (chunk_id == dash_level_props_chunk_id) {
            auto& dash_level_props = level.GetDashProps();
            int version = file.read<std::uint32_t>();
            if (version == 1) {
                dash_level_props.Deserialize(file);
            } else {
                file.seek(chunk_size - 4, rf::File::seek_cur);
            }
            regs.eip = 0x0043090C;
        }
    },
};

CodeInjection CRedLevel_SaveLevel_inj{
    0x00430CBD,
    [](auto& regs) {
        auto& level = *static_cast<CRedLevel*>(regs.edi);
        auto& file = *static_cast<rf::File*>(regs.esi);
        auto start_pos = level.BeginRflSection(file, dash_level_props_chunk_id);
        auto& dash_level_props = level.GetDashProps();
        file.write<std::uint32_t>(1);
        dash_level_props.Serialize(file);
        level.EndRflSection(file, start_pos);
    },
};

CodeInjection CLevelDialog_OnInitDialog_inj{
    0x004676C0,
    [](auto& regs) {
        HWND hdlg = WndToHandle(regs.esi);
        auto& dash_level_props = CRedLevel::Get()->GetDashProps();
        CheckDlgButton(hdlg, IDC_LEVEL_LIGHTMAPS_FULL_DEPTH, dash_level_props.lightmaps_full_depth ? BST_CHECKED : BST_UNCHECKED);
    },
};

CodeInjection CLevelDialog_OnOK_inj{
    0x00468470,
    [](auto& regs) {
        HWND hdlg = WndToHandle(regs.ecx);
        auto& dash_level_props = CRedLevel::Get()->GetDashProps();
        dash_level_props.lightmaps_full_depth = IsDlgButtonChecked(hdlg, IDC_LEVEL_LIGHTMAPS_FULL_DEPTH) == BST_CHECKED;
    },
};

void ApplyLevelPatches()
{
    // Save/load additional level properties
    write_mem<std::uint32_t>(0x0041C906 + 1, 0x668 + sizeof(LevelDashProps));
    CRedLevel_ct_inj.install();
    CRedLevel_LoadLevel_inj0.install();
    CRedLevel_LoadLevel_inj1.install();
    CRedLevel_SaveLevel_inj.install();
    CLevelDialog_OnInitDialog_inj.install();
    CLevelDialog_OnOK_inj.install();

    // Disable lightmap clamping
    write_mem<std::uint8_t>(0x004A2ED2 + 1, 0);
    write_mem<std::uint8_t>(0x004A2EE0 + 1, 0);
    write_mem<std::uint8_t>(0x004A2EEA + 1, 0);

    // Increase default ambient color because lightmaps are no longer clamped
    constexpr std::uint8_t default_ambient_light = 64;
    write_mem<std::uint8_t>(0x0041CABD + 1, default_ambient_light);
    write_mem<std::uint8_t>(0x0041CABF + 1, default_ambient_light);
    write_mem<std::uint8_t>(0x0041CAC1 + 1, default_ambient_light);
    write_mem<std::uint8_t>(0x0041CAD3 + 1, default_ambient_light);
    write_mem<std::uint8_t>(0x0041CAD5 + 1, default_ambient_light);
    write_mem<std::uint8_t>(0x0041CAD7 + 1, default_ambient_light);
}
