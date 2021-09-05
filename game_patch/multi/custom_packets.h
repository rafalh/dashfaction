#pragma once

#include "../purefaction/pf_packets.h"

namespace rf {
    struct NetAddr;
    struct Player;
}    

namespace custom_packets { 
    enum class custom_packet_type : uint8_t {
        configure_hitsounds = 200,
        hitsound = 201,
    };

    struct configure_hitsounds_packet {
        rf_packet_header hdr;
        uint8_t level;
    };

    void send_hitsound_packet(rf::Player* player);
    void process_hitsound_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player);
    void send_configure_hitsounds_packet(rf::Player* player);
    void process_configure_hitsounds_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player);
    bool process_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player);
}