#include "edge_vox/core/client.hpp"

#include <atomic>
#include <stdexcept>
#include <thread>

#include "../net/control_client.hpp"
#include "../net/rtp_streamer.hpp"
#include "edge_vox/audio/audio_async.hpp"

class EdgeVoxClient::Impl {
public:
    Impl() : is_connected_(false), is_streaming_(false), audio_(30 * 1000) {  // 30 second buffer

        // This will run in your audio worker thread from SDL
        audio_process_timer_ = SDL_AddTimer(
            10,
            [](Uint32 interval, void* param) -> Uint32 {
                auto* self = static_cast<Impl*>(param);

                if (self->is_streaming_ && self->rtp_streamer_.is_active()) {
                    std::vector<float> samples;
                    self->audio_.get(10, samples);  // Get last 10ms of audio
                    if (!samples.empty()) {
                        self->rtp_streamer_.send_audio(samples);
                    }
                }

                return interval;
            },
            this);

        // Set up control client callback
        control_.set_status_callback([this](const std::string& status) {
            if (status_callback_) {
                status_callback_(status);
            }
        });
    }

    ~Impl() {
        if (audio_process_timer_) {
            SDL_RemoveTimer(audio_process_timer_);
        }
    }

    bool connect(const std::string& server_ip, uint16_t port) {
        if (is_connected_) {
            return true;
        }

        try {
            // Initialize RTP streamer
            if (!rtp_streamer_.init(server_ip, stream_config_.rtp_port,
                                    stream_config_.packet_size)) {
                return false;
            }

            // Connect control client
            if (!control_.connect(server_ip, stream_config_.control_port)) {
                return false;
            }

            // Initialize audio
            if (!audio_.init(-1, audio_config_.sample_rate)) {
                control_.disconnect();
                return false;
            }

            is_connected_ = true;
            return true;
        } catch (const std::exception& e) {
            if (status_callback_) {
                status_callback_(std::string("Connection error: ") + e.what());
            }
            return false;
        }
    }

    bool start_audio_stream() {
        if (!is_connected_) {
            return false;
        }

        if (is_streaming_) {
            return true;
        }

        if (!rtp_streamer_.start()) {
            return false;
        }

        audio_.clear();
        if (!audio_.resume()) {
            rtp_streamer_.stop();
            return false;
        }

        is_streaming_ = true;
        return true;
    }

    void stop_audio_stream() {
        if (!is_streaming_) {
            return;
        }

        audio_.pause();
        rtp_streamer_.stop();
        is_streaming_ = false;
    }

private:
    audio_async audio_;
    EdgeVoxRtpStreamer rtp_streamer_;
    EdgeVoxControlClient control_;
    SDL_TimerID audio_process_timer_;

    EdgeVoxAudioConfig audio_config_;
    EdgeVoxStreamConfig stream_config_;
    EdgeVoxClient::StatusCallback status_callback_;

    std::atomic<bool> is_connected_;
    std::atomic<bool> is_streaming_;
};

// class EdgeVoxClient::Impl {
// public:
//     Impl() : is_connected_(false), is_streaming_(false) {
//         // Set up audio callback for both streaming and wake word detection
//         audio_.set_data_callback([this](const std::vector<float>& samples) {
//             // Handle streaming
//             if (is_streaming_ && rtp_streamer_.is_active()) {
//                 rtp_streamer_.send_audio(samples);
//             }

//             // Process wake word detection
//             if (wake_word_callback_) {
//                 // TODO: Add wake word detection logic here
//                 // if (wake_word_detected) {
//                 //     wake_word_callback_();
//                 // }
//             }
//         });

//         // Set up control client callback
//         control_.set_status_callback([this](const std::string& status) {
//             if (status_callback_) {
//                 status_callback_(status);
//             }
//         });
//     }

//     bool connect(const std::string& server_ip, uint16_t port) {
//         if (is_connected_) {
//             return true;
//         }

//         try {
//             // Initialize RTP streamer
//             if (!rtp_streamer_.init(server_ip, stream_config_.rtp_port,
//                                     stream_config_.packet_size)) {
//                 return false;
//             }

//             // Connect control client
//             if (!control_.connect(server_ip, stream_config_.control_port)) {
//                 return false;
//             }

//             // Initialize audio processor
//             if (!audio_.init(-1, audio_config_.sample_rate)) {
//                 control_.disconnect();
//                 return false;
//             }

//             is_connected_ = true;
//             return true;
//         } catch (const std::exception& e) {
//             if (status_callback_) {
//                 status_callback_(std::string("Connection error: ") + e.what());
//             }
//             return false;
//         }
//     }

//     void disconnect() {
//         if (!is_connected_) {
//             return;
//         }

//         stop_audio_stream();
//         control_.disconnect();
//         is_connected_ = false;
//     }

//     bool is_connected() const {
//         return is_connected_;
//     }

//     bool start_audio_stream() {
//         if (!is_connected_) {
//             return false;
//         }

//         if (is_streaming_) {
//             return true;
//         }

//         if (!rtp_streamer_.start()) {
//             return false;
//         }

//         audio_.start();
//         is_streaming_ = true;
//         return true;
//     }

//     void stop_audio_stream() {
//         if (!is_streaming_) {
//             return;
//         }

//         audio_.stop();
//         rtp_streamer_.stop();
//         is_streaming_ = false;
//     }

//     bool is_streaming() const {
//         return is_streaming_;
//     }

//     void set_audio_config(const EdgeVoxAudioConfig& config) {
//         if (is_streaming_) {
//             throw std::runtime_error("Cannot change audio config while streaming");
//         }
//         audio_config_ = config;
//     }

//     void set_stream_config(const EdgeVoxStreamConfig& config) {
//         if (is_connected_) {
//             throw std::runtime_error("Cannot change stream config while connected");
//         }
//         stream_config_ = config;
//     }

//     void set_status_callback(EdgeVoxClient::StatusCallback callback) {
//         status_callback_ = std::move(callback);
//     }

//     void set_wake_word_callback(EdgeVoxClient::WakeWordCallback callback) {
//         wake_word_callback_ = std::move(callback);
//     }

// private:
//     EdgeVoxAudioProcessor audio_;
//     EdgeVoxRtpStreamer rtp_streamer_;
//     EdgeVoxControlClient control_;

//     EdgeVoxAudioConfig audio_config_;
//     EdgeVoxStreamConfig stream_config_;
//     EdgeVoxClient::StatusCallback status_callback_;
//     EdgeVoxClient::WakeWordCallback wake_word_callback_;

//     std::atomic<bool> is_connected_;
//     std::atomic<bool> is_streaming_;
// };

// // EdgeVoxClient implementation
// EdgeVoxClient::EdgeVoxClient() : pimpl_(std::make_unique<Impl>()) {}
// EdgeVoxClient::~EdgeVoxClient() = default;

// bool EdgeVoxClient::connect(const std::string& server_ip, uint16_t port) {
//     return pimpl_->connect(server_ip, port);
// }

// void EdgeVoxClient::disconnect() {
//     pimpl_->disconnect();
// }

// bool EdgeVoxClient::is_connected() const {
//     return pimpl_->is_connected();
// }

// bool EdgeVoxClient::start_audio_stream() {
//     return pimpl_->start_audio_stream();
// }

// void EdgeVoxClient::stop_audio_stream() {
//     pimpl_->stop_audio_stream();
// }

// bool EdgeVoxClient::is_streaming() const {
//     return pimpl_->is_streaming();
// }

// void EdgeVoxClient::set_audio_config(const EdgeVoxAudioConfig& config) {
//     pimpl_->set_audio_config(config);
// }

// void EdgeVoxClient::set_stream_config(const EdgeVoxStreamConfig& config) {
//     pimpl_->set_stream_config(config);
// }

// void EdgeVoxClient::set_status_callback(StatusCallback callback) {
//     pimpl_->set_status_callback(std::move(callback));
// }

// void EdgeVoxClient::set_wake_word_callback(WakeWordCallback callback) {
//     pimpl_->set_wake_word_callback(std::move(callback));
// }