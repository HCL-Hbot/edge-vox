#include "rtp_receiver.hpp"

#include <iostream>
#include <regex>
#include <uvgrtp/lib.hh>

class EdgeVoxRtpReceiver::Impl {
public:
    Impl() : active_(false), session_(nullptr), stream_(nullptr) {}

    ~Impl() {
        stop();
    }

    static void rtp_receive_hook(void* arg, uvgrtp::frame::rtp_frame* frame) {
        auto* self = static_cast<Impl*>(arg);
        if (!self->callback_ || !frame || !frame->payload || frame->payload_len == 0) {
            uvgrtp::frame::dealloc_frame(frame);
            return;
        }

        const size_t num_samples = frame->payload_len / 2;  // 2 bytes per sample
        std::vector<float> samples(num_samples);

        for (size_t i = 0; i < num_samples; i++) {
            uint16_t msb = frame->payload[i * 2];
            uint16_t lsb = frame->payload[i * 2 + 1];
            int16_t pcm = (msb << 8) | lsb;
            samples[i] = pcm / 32767.0f;
        }

        self->callback_(samples);
        uvgrtp::frame::dealloc_frame(frame);
    }

    bool init(const std::string& local_ip, uint16_t port, int flags = 0) {
        try {
            // Validate IP and port
            if (!isValidIPAddress(local_ip) || port == 0) {
                return false;
            }

            if (ctx_) {
                stop();
            }

            ctx_ = std::make_unique<uvgrtp::context>();
            session_ = ctx_->create_session(local_ip);
            port_ = port;
            flags_ = flags;
            std::cout << "Creating stream, flags:" << flags_ << std::endl;

            if (!session_) {
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    bool start() {
        if (active_) {
            return true;
        }

        try {
            std::cout << "Creating stream, flags:" << flags_ << std::endl;
            stream_ = session_->create_stream(port_, RTP_FORMAT_GENERIC, flags_);

            if (!stream_) {
                return false;
            }

            if (stream_->install_receive_hook(this, rtp_receive_hook) != RTP_OK) {
                session_->destroy_stream(stream_);
                stream_ = nullptr;
                return false;
            }

            active_ = true;
            return true;
        } catch (const std::exception& e) {
            if (stream_) {
                session_->destroy_stream(stream_);
                stream_ = nullptr;
            }
            return false;
        }
    }

    void stop() {
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
        ctx_.reset();
        active_ = false;
    }

    bool is_active() const {
        return active_;
    }

    void set_audio_callback(AudioCallback callback) {
        callback_ = std::move(callback);
    }

private:
    bool isValidIPAddress(const std::string& ip) {
        std::regex ipv4_pattern(
            "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
            "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
        return std::regex_match(ip, ipv4_pattern) || ip == "localhost";
    }

    std::unique_ptr<uvgrtp::context> ctx_;
    uvgrtp::session* session_;
    uvgrtp::media_stream* stream_;
    bool active_;
    uint16_t port_;
    int flags_;
    AudioCallback callback_;
};

EdgeVoxRtpReceiver::EdgeVoxRtpReceiver() : pimpl_(std::make_unique<Impl>()) {}
EdgeVoxRtpReceiver::~EdgeVoxRtpReceiver() = default;

bool EdgeVoxRtpReceiver::init(const std::string& local_ip, uint16_t port, int flags) {
    return pimpl_->init(local_ip, port, flags);
}

bool EdgeVoxRtpReceiver::start() {
    return pimpl_->start();
}

void EdgeVoxRtpReceiver::stop() {
    pimpl_->stop();
}

bool EdgeVoxRtpReceiver::is_active() const {
    return pimpl_->is_active();
}

void EdgeVoxRtpReceiver::set_audio_callback(AudioCallback callback) {
    pimpl_->set_audio_callback(std::move(callback));
}
