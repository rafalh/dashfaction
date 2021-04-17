#include <algorithm>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>

int multi_tdm_red_score = 0;
int multi_tdm_blue_score = 0;

static FunHook<void()> multi_tdm_level_init_hook{
    0x00482880,
    []() {
        multi_tdm_red_score = 0;
        multi_tdm_blue_score = 0;
    },
};

static FunHook<void(unsigned short)> multi_tdm_set_red_score_hook{
    0x00482890,
    [](unsigned short score) {
        multi_tdm_red_score = score;
    },
};

static FunHook<void(unsigned short)> multi_tdm_set_blue_score_hook{
    0x004828C0,
    [](unsigned short score) {
        multi_tdm_blue_score = score;
    },
};

static FunHook<int()> multi_tdm_get_red_team_score_hook{
    0x004828F0,
    []() {
        return multi_tdm_red_score;
    },
};

static FunHook<int()> multi_tdm_get_blue_team_score_hook{
    0x00482900,
    []() {
        return multi_tdm_blue_score;
    },
};

static FunHook<void()> multi_tdm_increase_red_score_hook{
    0x00482910,
    []() {
        ++multi_tdm_red_score;
    },
};

static FunHook<void()> multi_tdm_increase_blue_score_hook{
    0x00482920,
    []() {
        ++multi_tdm_blue_score;
    },
};

static FunHook<int()> multi_ctf_get_red_team_score_hook{
    0x00475020,
    []() {
        // Original code uses ubyte return type so make sure 24 higher bytes are 0
        return multi_ctf_get_red_team_score_hook.call_target() & 0xFF;
    },
};

static FunHook<int()> multi_ctf_get_blue_team_score_hook{
    0x00475030,
    []() {
        // Original code uses ubyte return type so make sure 24 higher bytes are 0
        return multi_ctf_get_blue_team_score_hook.call_target() & 0xFF;
    },
};

static CallHook<void(int*, int, int)> multi_load_dedicated_server_config_max_kills_cap_int_hook{
    0x0046DECF,
    [](int *v, int, int) {
        *v = std::clamp(*v, 1, 999);
    },
};

void multi_tdm_apply_patch()
{
    // Use 16 bit values instead of 8 bit for storing Team DM scores
    multi_tdm_level_init_hook.install();
    multi_tdm_set_red_score_hook.install();
    multi_tdm_set_blue_score_hook.install();
    multi_tdm_get_red_team_score_hook.install();
    multi_tdm_get_blue_team_score_hook.install();
    multi_tdm_increase_red_score_hook.install();
    multi_tdm_increase_blue_score_hook.install();

    // Change return type of multi_ctf_get_*_team_score from ubyte to int to handle cases when
    // tdm and ctf code paths are shared
    multi_ctf_get_red_team_score_hook.install();
    multi_ctf_get_blue_team_score_hook.install();

    // Patch assembly code that assumes ubyte return types
    using namespace asm_regs;
    // multi_check_for_round_end
    AsmWriter{0x0046E80B}.nop(5); // and eax, 0xFF
    AsmWriter{0x0046E823}.nop(5); // and eax, 0xFF
    // draw_scoreboard_internal
    AsmWriter{0x00470B4E}.nop(5); // and eax, 0xFF
    AsmWriter{0x00470B66}.nop(5); // and eax, 0xFF
    // send_team_scores_packet
    AsmWriter{0x00472169, 0x0047216D}.mov(si, ax); // movzx si, al
    AsmWriter{0x00472172, 0x00472176}.nop(); // movzx ax, al
    // multi_hud_render_team_scores
    AsmWriter{0x00477BCD}.nop(5); // and eax, 0xFF
    AsmWriter{0x00477BD9}.nop(5); // and eax, 0xFF
    // multi_limbo_init
    AsmWriter{0x0047C300, 0x0047C302}.mov(ebx, eax); // mov bl, al
    AsmWriter{0x0047C307, 0x0047C309}.cmp(eax, ebx); // cmp al, bl
    AsmWriter{0x0047C318, 0x0047C309}.mov(ebx, eax); // mov bl, al
    AsmWriter{0x0047C34B, 0x0047C34D}.mov(ebx, eax); // mov bl, al
    AsmWriter{0x0047C352, 0x0047C354}.cmp(eax, ebx); // cmp al, bl
    // Note: multi_hud_render_scoreboard_old (0x00478150) is not used

    // Change the range of allowed values for Max Kills dedicated server config option to 1-999
    multi_load_dedicated_server_config_max_kills_cap_int_hook.install();
}
