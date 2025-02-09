
#include "rtp_streamer.hpp"

#include <cstring>
#include <iostream>
#include <regex>
#include <uvgrtp/lib.hh>

class EdgeVoxRtpStreamer::Impl {
public:
    Impl() : active_(false), session_(nullptr), stream_(nullptr) {}

    ~Impl() {
        stop();
    }

    bool init(const std::string& host, uint16_t port, uint32_t payload_size) {
        try {
            // Validate parameters
            if (!isValidIPAddress(host)) {
                std::cerr << "Invalid IP address format" << std::endl;
                return false;
            }

            if (port == 0) {
                std::cerr << "Invalid port number" << std::endl;
                return false;
            }

            if (payload_size == 0) {
                std::cerr << "Invalid payload size" << std::endl;
                return false;
            }

            host_ = host;
            port_ = port;
            payload_size_ = payload_size;

            // Create a global RTP context object
            ctx_ = std::make_unique<uvgrtp::context>();

            // Create a session with the remote address
            session_ = ctx_->create_session(host_);
            if (!session_) {
                std::cerr << "Failed to create RTP session" << std::endl;
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            std::cerr << "RTP init error: " << e.what() << std::endl;
            return false;
        }
    }

    bool start() {
        try {
            if (active_) {
                return true;
            }

            if (!session_) {
                std::cerr << "Cannot start: session not initialized" << std::endl;
                return false;
            }

            // Create a media stream with the specified port
            int flags = RCE_SEND_ONLY;
            stream_ = session_->create_stream(port_, RTP_FORMAT_GENERIC, flags);

            if (!stream_) {
                std::cerr << "Failed to create media stream" << std::endl;
                return false;
            }

            active_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Start error: " << e.what() << std::endl;
            return false;
        }
    }

    void stop() {
        try {
            if (stream_) {
                if (session_) {
                    session_->destroy_stream(stream_);
                }
                stream_ = nullptr;
            }
            if (session_) {
                if (ctx_) {
                    ctx_->destroy_session(session_);
                }
                session_ = nullptr;
            }
            active_ = false;
        } catch (const std::exception& e) {
            std::cerr << "Stop error: " << e.what() << std::endl;
        }
    }

    bool send_audio(const std::vector<float>& samples) {
        if (!active_ || !stream_) {
            return false;
        }

        if (samples.empty()) {
            std::cerr << "Failed to send RTP frame" << std::endl;
            return false;
        }

        try {
            // Convert float samples to int16_t for network transmission
            std::vector<uint8_t> audio_data;
            audio_data.reserve(samples.size() * sizeof(int16_t));

            for (float sample : samples) {
                int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
                audio_data.push_back((pcm >> 8) & 0xFF);
                audio_data.push_back(pcm & 0xFF);
            }

            // Create a unique_ptr for the frame data as required by uvgRTP
            auto frame = std::make_unique<uint8_t[]>(audio_data.size());
            std::memcpy(frame.get(), audio_data.data(), audio_data.size());

            // Send audio data using uvgRTP
            int ret = stream_->push_frame(std::move(frame), audio_data.size(), RTP_NO_FLAGS);
            if (ret != RTP_OK) {
                std::cerr << "Failed to send RTP frame" << std::endl;
                return false;
            }
            return true;

        } catch (const std::exception& e) {
            std::cerr << "RTP send error: " << e.what() << std::endl;
            return false;
        }
    }

    bool is_active() const {
        return active_;
    }

private:
    bool isValidIPAddress(const std::string& ip) {
        // Simple IPv4 validation using regex
        std::regex ipv4_pattern(
            "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
            "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");

        return std::regex_match(ip, ipv4_pattern) || ip == "localhost";
    }

    std::unique_ptr<uvgrtp::context> ctx_;
    uvgrtp::session* session_;
    uvgrtp::media_stream* stream_;
    bool active_;

    // Store configuration
    std::string host_;
    uint16_t port_;
    uint32_t payload_size_;
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