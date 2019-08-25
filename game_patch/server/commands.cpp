#include "../commands.h"

void ExtendRoundTime(int minutes)
{
    auto& level_time = AddrAsRef<float>(0x006460F0);
    level_time -= minutes * 60.0f;
    std::string msg = StringFormat("Round extended by %d minutes", minutes);
    rf::ChatSay(msg.c_str(), false);
}

DcCommand2 map_ext_cmd{
    "map_ext",
    [](std::optional<int> minutes_opt) {
        if (!rf::is_local_net_game) {
            rf::DcPrint("Command can be only executed on server", nullptr);
            return;
        }
        int minutes = minutes_opt.value_or(5);
        ExtendRoundTime(minutes);
    },
    "Extend round time",
    "map_ext [minutes]",
};

void InitServerCommands()
{
    map_ext_cmd.Register();
}
