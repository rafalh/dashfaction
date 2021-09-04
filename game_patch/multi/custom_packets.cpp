#include <array>
#include <xlog/xlog.h>
#include "../main/main.h"
#include "../rf/multi.h"
#include "../rf/sound/sound.h"
#include "../rf/player/player.h"
#include "../misc/player.h"
#include "../multi/multi.h"
#include "../purefaction/pf.h"
#include "custom_packets.h"

namespace custom_packets {
    // Do not drop hitsound packets to save bandwidth, since hitsound packets are
    //  significantly smaller than sound packets.
    void send_hitsound_packet(rf::Player* player){
        if (!rf::is_server) {
            return;
        }
        
        rf_packet_header packet{};
        packet.type = static_cast<uint8_t>(custom_packet_type::hitsound);
        packet.size = 0;

        rf::multi_io_send(player, &packet, sizeof(packet));
    }

    void process_hitsound_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player) {
        if (rf::is_server) {
            return;
        }

        rf_packet_header packet;
        if (len < sizeof(packet)) {
            xlog::trace("Cannot process a hitsound packet; invalid length");
            return;
        }

        float invalid = std::numeric_limits<float>::quiet_NaN();
        rf::Vector3 position{invalid, invalid, invalid};
        rf::snd_play_3d(29, position, 1.0f, rf::zero_vector, 0);
    }

    void send_configure_hitsounds_packet(rf::Player* player) {
        if (rf::is_server) {
            return;
        }

        configure_hitsounds_packet packet{};
        packet.hdr.type = static_cast<uint8_t>(custom_packet_type::configure_hitsounds);
        packet.hdr.size = sizeof(packet) - sizeof(packet.hdr);

        if (g_game_config.multi_hitsounds_legacy_mode) {
            packet.level = 0;
        } 
        else if (g_game_config.multi_hitsounds) {
            packet.level = 2;
        }
        else {
            packet.level = 1;
        }

        rf::multi_io_send_reliable(player, &packet, sizeof(packet), 0);
    }

    void process_configure_hitsounds_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player) {
        if (!rf::is_server) {
            return;
        } 

        configure_hitsounds_packet packet;
        if (len < sizeof(packet)) {
            xlog::trace("Cannot process a configure_hitsounds packet; invalid length");
            return;
        }

        std::memcpy(&packet, data, sizeof(packet));

        if (packet.level > 2) {
            xlog::trace("Cannot process a configure_hitsounds packet; invalid level");
            return;
        }

        PlayerAdditionalData& pdata = get_player_additional_data(player);
        pdata.multi_hitsounds = packet.level;
        
        constexpr int NETPLAYER_STATE_IN_GAME = 2; 
        if (player->net_data->state == NETPLAYER_STATE_IN_GAME) {
            static const std::array<std::string, 3> messages {  
                "\xA6 Hitsounds are set to legacy mode", 
                "\xA6 Hitsounds are disabled",
                "\xA6 Hitsounds are enabled",
            };
            send_chat_line_packet(messages[pdata.multi_hitsounds].c_str(), player);
        }
    }

    bool process_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player) {
        rf_packet_header header;
        if (len < static_cast<int>(sizeof(header))) {
            xlog::trace("Cannot process a custom packet; invalid length");
            return false;
        }

        std::memcpy(&header, data, sizeof(header));

        switch (static_cast<custom_packet_type>(header.type)) {
            case custom_packet_type::configure_hitsounds: {
                process_configure_hitsounds_packet(data, len, addr, player);
                return true;
            }
            case custom_packet_type::hitsound: {
                process_hitsound_packet(data, len, addr, player);
                return true;
            }
        }

        return false;
    }
}