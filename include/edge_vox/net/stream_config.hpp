#pragma once
#include <string>

struct EdgeVoxStreamConfig {
    std::string server_ip;
    uint16_t rtp_port{5004};
    uint16_t control_port{1883};
    uint32_t packet_size{512};
    std::string control_topic{"status/server"};
};