#include <regex>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include "multi.h"
#include "multi_private.h"
#include "../misc/misc.h"
#include "../rf/os/os.h"
#include "../rf/multi.h"
#include "../rf/os/console.h"

// Note: this must be called from DLL init function
// Note: we can't use global variable because that would lead to crash when launcher loads this DLL to check dependencies
static rf::CmdLineParam& get_url_cmd_line_param()
{
    static rf::CmdLineParam url_param{"-url", "", true};
    return url_param;
}

void handle_url_param()
{
    if (!get_url_cmd_line_param().found()) {
        return;
    }

    auto url = get_url_cmd_line_param().get_arg();
    std::regex e("^rf://([\\w\\.-]+):(\\d+)/?(?:\\?password=(.*))?$");
    std::cmatch cm;
    if (!std::regex_match(url, cm, e)) {
        xlog::warn("Unsupported URL: %s", url);
        return;
    }

    auto host_name = cm[1].str();
    auto port = static_cast<uint16_t>(std::stoi(cm[2].str()));
    auto password = cm[3].str();

    auto hp = gethostbyname(host_name.c_str());
    if (!hp) {
        xlog::warn("URL host lookup failed");
        return;
    }

    if (hp->h_addrtype != AF_INET) {
        xlog::warn("Unsupported address type (only IPv4 is supported)");
        return;
    }

    rf::console_printf("Connecting to %s:%d...", host_name.c_str(), port);
    auto host = ntohl(reinterpret_cast<in_addr *>(hp->h_addr_list[0])->S_un.S_addr);

    rf::NwAddr addr{host, port};
    start_join_multi_game_sequence(addr, password);
}

FunHook<void()> multi_limbo_init{
    0x0047C280,
    []() {
        multi_limbo_init.call_target();
        server_on_limbo_state_enter();
    },
};

void multi_init_player(rf::Player* player)
{
    multi_kill_init_player(player);
}

void multi_do_patch()
{
    multi_kill_do_patch();
    level_download_do_patch();
    network_init();

    level_download_init();

    // Init cmd line param
    get_url_cmd_line_param();
}

void multi_after_full_game_init()
{
    handle_url_param();
}
