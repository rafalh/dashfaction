#include <functional>
#include <map>
#include <optional>
#include <string>
#include <common/utils/string-utils.h>
#include "../rf/math/vector.h"
#include "../rf/math/matrix.h"
#include "../rf/os/timestamp.h"
#include "../purefaction/pf_packets.h"

// Forward declarations
namespace rf
{
    struct Player;
}

struct PlayerNetGameSaveData
{
    rf::Vector3 pos;
    rf::Matrix3 orient;
};

struct PlayerAdditionalData
{
    std::optional<pf_pure_status> received_ac_status{};
    bool is_browser = false;
    bool is_muted = false;
    int last_hitsound_sent_ms = 0;
    std::map<std::string, PlayerNetGameSaveData> saves;
    rf::Vector3 last_teleport_pos;
    rf::TimestampRealtime last_teleport_timestamp;
};

void find_player(const StringMatcher& query, std::function<void(rf::Player*)> consumer);
void reset_player_additional_data(const rf::Player* player);
PlayerAdditionalData& get_player_additional_data(rf::Player* player);
