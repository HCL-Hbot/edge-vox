#include "rtp_streamer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <cstring>
#include <thread>

class EdgeVoxRtpStreamer::Impl {
public:
    Impl() : socket_(-1), sequence_number_(0), active_(false) {}

    bool init(const std::string& host, uint16_t port, uint32_t payload_size) {
        host_ = host;
        port_ = port;
        payload_size_ = payload_size;

        socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_ < 0) {
            return false;
        }

        memset(&dest_addr_, 0, sizeof(dest_addr_));
        dest_addr_.sin_family = AF_INET;
        dest_addr_.sin_port = htons(port_);
        inet_pton(AF_INET, host_.c_str(), &dest_addr_.sin_addr);

        return true;
    }

    bool start() {
        if (active_)
            return true;
        if (socket_ < 0)
            return false;
        active_ = true;
        return true;
    }

    void stop() {
        active_ = false;
        if (socket_ >= 0) {
            shutdown(socket_, SHUT_RDWR);
            socket_ = -1;
        }
    }

    bool send_audio(const std::vector<float>& samples) {
        if (!active_ || socket_ < 0)
            return false;

        // Convert float samples to network bytes and add RTP header
        std::vector<uint8_t> packet(12 + samples.size() * sizeof(int16_t));

        // RTP header (simplified)
        packet[0] = 0x80;  // Version 2
        packet[1] = 0x00;  // No padding, no extension, no CSRC
        packet[2] = (sequence_number_ >> 8) & 0xFF;
        packet[3] = sequence_number_ & 0xFF;

        // Convert samples to int16_t and copy to packet
        for (size_t i = 0; i < samples.size(); ++i) {
            int16_t sample = static_cast<int16_t>(samples[i] * 32767.0f);
            packet[12 + i * 2] = (sample >> 8) & 0xFF;
            packet[12 + i * 2 + 1] = sample & 0xFF;
        }

        ssize_t sent = sendto(socket_, packet.data(), packet.size(), 0,
                              (struct sockaddr*)&dest_addr_, sizeof(dest_addr_));

        sequence_number_++;
        return sent == static_cast<ssize_t>(packet.size());
    }

    bool is_active() const {
        return active_;
    }

private:
    std::string host_;
    uint16_t port_;
    uint32_t payload_size_;
    int socket_;
    uint16_t sequence_number_;
    struct sockaddr_in dest_addr_;
    std::atomic<bool> active_;
};

EdgeVoxRtpStreamer::EdgeVoxRtpStreamer() : pimpl_(std::make_unique<Impl>()) {}
EdgeVoxRtpStreamer::~EdgeVoxRtpStreamer() = default;

bool EdgeVoxRtpStreamer::init(const std::string& host, uint16_t port, uint32_t payload_size) {
    return pimpl_->init(host, port, payload_size);
}

bool EdgeVoxRtpStreamer::start() {
    return pimpl_->start();
}

void EdgeVoxRtpStreamer::stop() {
    pimpl_->stop();
}

bool EdgeVoxRtpStreamer::send_audio(const std::vector<float>& samples) {
    return pimpl_->send_audio(samples);
}

bool EdgeVoxRtpStreamer::is_active() const {
    return pimpl_->is_active();
}