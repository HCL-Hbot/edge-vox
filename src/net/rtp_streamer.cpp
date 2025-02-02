#include "net/rtp_streamer.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <thread>

#include "rtp_packet.hpp"

class EdgeVoxRtpStreamer::Impl {
public:
    Impl() : socket_(-1), active_(false) {
        packet_ = std::make_unique<RtpPacket>();
    }

    bool init(const std::string& host, uint16_t port, uint32_t payload_size) {
        if (host.empty()) {
            return false;
        }

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

        // Properly check inet_pton result
        int result = inet_pton(AF_INET, host_.c_str(), &dest_addr_.sin_addr);
        if (result <= 0) {
            stop();
            return false;
        }

        // Set socket options for real-time streaming
        int optval = 1;
        if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            return false;
        }

        return true;
    }

    bool start() {
        if (active_) {
            return true;
        }

        // If socket was closed, recreate it
        if (socket_ < 0) {
            if (!init(host_, port_, payload_size_)) {
                return false;
            }
        }

        active_ = true;
        return true;
    }

    void stop() {
        active_ = false;
        if (socket_ >= 0) {
            shutdown(socket_, SHUT_RDWR);
            close(socket_);
            socket_ = -1;
        }
    }

    bool send_audio(const std::vector<float>& samples) {
        if (!active_ || socket_ < 0) {
            return false;
        }

        // Convert float samples to network bytes
        std::vector<uint8_t> audio_data;
        audio_data.reserve(samples.size() * sizeof(int16_t));

        for (float sample : samples) {
            int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
            audio_data.push_back((pcm >> 8) & 0xFF);
            audio_data.push_back(pcm & 0xFF);
        }

        // Set the payload
        packet_->setPayload(audio_data);

        // Set marker bit if this is the first packet in a talkspurt
        if (samples_sent_ == 0) {
            packet_->setMarker(true);
        } else {
            packet_->setMarker(false);
        }

        // Update timestamp based on number of samples
        packet_->incrementTimestamp(samples.size());

        // Serialize the packet
        auto packet_data = packet_->serialize();

        // Send the packet
        ssize_t sent = sendto(socket_, packet_data.data(), packet_data.size(), 0,
                              (struct sockaddr*)&dest_addr_, sizeof(dest_addr_));

        if (sent == static_cast<ssize_t>(packet_data.size())) {
            packet_->incrementSequenceNumber();
            samples_sent_ += samples.size();
            return true;
        }

        return false;
    }

    bool is_active() const {
        return active_;
    }

private:
    std::string host_;
    uint16_t port_;
    uint32_t payload_size_;
    int socket_;
    struct sockaddr_in dest_addr_;
    std::atomic<bool> active_;
    std::unique_ptr<RtpPacket> packet_;
    uint64_t samples_sent_{0};
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
