#include <windowsx.h>
#include <patch_common/FunHook.h>
#include <string>
#include "mfc_types.h"
#include "resources.h"

constexpr char TRIGGER_PF_FLAGS_PREFIX = '\xAB';
constexpr uint8_t TRIGGER_CLIENT_SIDE = 0x2;
constexpr uint8_t TRIGGER_SOLO = 0x4;
constexpr uint8_t TRIGGER_TELEPORT = 0x8;

class TriggerCustomFlagsHelper {
    HWND hwnd_;
    bool is_solo_ = false;
    bool is_clientside_ = false;
    bool is_teleport_ = false;

public:
    TriggerCustomFlagsHelper(HWND hwnd) : hwnd_(hwnd) {}

    void PullCheckBoxesStates()
    {
        is_solo_ = Button_GetCheck(GetDlgItem(hwnd_, IDC_TRIGGER_SOLO)) == BST_CHECKED;
        is_clientside_ = Button_GetCheck(GetDlgItem(hwnd_, IDC_TRIGGER_CLIENTSIDE)) == BST_CHECKED;
        is_teleport_ = Button_GetCheck(GetDlgItem(hwnd_, IDC_TRIGGER_TELEPORT)) == BST_CHECKED;
    }

    void UpdateCheckBoxesStates()
    {
        CheckDlgButton(hwnd_, IDC_TRIGGER_SOLO, is_solo_ ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd_, IDC_TRIGGER_CLIENTSIDE, is_clientside_ ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd_, IDC_TRIGGER_TELEPORT, is_teleport_ ? BST_CHECKED : BST_UNCHECKED);
    }

    void ExtractFlagsAndRemoveFromScriptName()
    {
        char script_name[256];
        GetDlgItemTextA(hwnd_, IDC_TRIGGER_SCRIPT_NAME, script_name, sizeof(script_name));
        if (script_name[0] == TRIGGER_PF_FLAGS_PREFIX) {
            uint8_t pf_flags = static_cast<uint8_t>(script_name[1]);
            is_solo_ = pf_flags & TRIGGER_SOLO;
            is_clientside_ = pf_flags & TRIGGER_CLIENT_SIDE;
            is_teleport_ = pf_flags & TRIGGER_TELEPORT;
            SetDlgItemTextA(hwnd_, IDC_TRIGGER_SCRIPT_NAME, script_name + 2);
        }
    }

    void PutFlagsIntoScriptName()
    {
        if (is_solo_ || is_clientside_ || is_teleport_) {
            char script_name[256];
            GetDlgItemTextA(hwnd_, IDC_TRIGGER_SCRIPT_NAME, script_name, sizeof(script_name));
            std::string new_script_name;
            new_script_name += TRIGGER_PF_FLAGS_PREFIX;
            uint8_t pf_flags = 0;
            pf_flags |= is_solo_ ? TRIGGER_SOLO : 0;
            pf_flags |= is_clientside_ ? TRIGGER_CLIENT_SIDE : 0;
            pf_flags |= is_teleport_ ? TRIGGER_TELEPORT : 0;
            new_script_name += static_cast<char>(pf_flags);
            new_script_name += script_name;
            SetDlgItemTextA(hwnd_, IDC_TRIGGER_SCRIPT_NAME, new_script_name.c_str());
        }

    }
};

void __fastcall CTriggerPropsDialog_DoDataExchange_new(CWnd* this_, int, CDataExchange* dx);
FunHook<decltype(CTriggerPropsDialog_DoDataExchange_new)> CTriggerPropsDialog_OnDataExchange_hook{
    0x00471C10,
    CTriggerPropsDialog_DoDataExchange_new,
};
void __fastcall CTriggerPropsDialog_DoDataExchange_new(CWnd* this_, int, CDataExchange* dx)
{
    HWND hwnd = WndToHandle(this_);
    TriggerCustomFlagsHelper flags_helper{hwnd};
    if (dx->m_bSaveAndValidate) {
        // Saving - put flags into the script name field
        flags_helper.PullCheckBoxesStates();
        flags_helper.PutFlagsIntoScriptName();
    }
    CTriggerPropsDialog_OnDataExchange_hook.CallTarget(this_, 0, dx);
    if (!dx->m_bSaveAndValidate) {
        // Loading - extract flags from the script name field
        flags_helper.ExtractFlagsAndRemoveFromScriptName();
        flags_helper.UpdateCheckBoxesStates();
    }
}

void ApplyTriggerPatches()
{
    // Add multiplayer settings in trigger properties dialog
    CTriggerPropsDialog_OnDataExchange_hook.Install();
}
