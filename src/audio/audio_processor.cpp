#include "audio_processor.hpp"

#include <atomic>
#include <mutex>
#include <queue>
#include <thread>

class EdgeVoxAudioProcessor::Impl {
public:
    Impl() : running_(false) {}

    bool init(int device_id, uint32_t sample_rate) {
        // Initialize audio device
        return true;
    }

    void start() {
        if (running_)
            return;
        running_ = true;
        process_thread_ = std::thread([this]() {
            while (running_) {
                process_audio();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    void stop() {
        running_ = false;
        if (process_thread_.joinable()) {
            process_thread_.join();
        }
    }

    void get_samples(std::vector<float>& samples) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (!audio_buffer_.empty()) {
            samples = audio_buffer_;
            audio_buffer_.clear();
        }
    }

    void set_data_callback(std::function<void(const std::vector<float>&)> callback) {
        data_callback_ = callback;
    }

private:
    void process_audio() {
        // Process audio data
        std::vector<float> buffer(1024, 0.0f);  // Dummy data for now
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            audio_buffer_ = buffer;
        }
        if (data_callback_) {
            data_callback_(buffer);
        }
    }

    std::atomic<bool> running_;
    std::thread process_thread_;
    std::vector<float> audio_buffer_;
    std::mutex buffer_mutex_;
    std::function<void(const std::vector<float>&)> data_callback_;
};

EdgeVoxAudioProcessor::EdgeVoxAudioProcessor() : pimpl_(std::make_unique<Impl>()) {}
EdgeVoxAudioProcessor::~EdgeVoxAudioProcessor() = default;

bool EdgeVoxAudioProcessor::init(int device_id, uint32_t sample_rate) {
    return pimpl_->init(device_id, sample_rate);
}

void EdgeVoxAudioProcessor::start() {
    pimpl_->start();
}

void EdgeVoxAudioProcessor::stop() {
    pimpl_->stop();
}

void EdgeVoxAudioProcessor::get_samples(std::vector<float>& samples) {
    pimpl_->get_samples(samples);
}

void EdgeVoxAudioProcessor::set_data_callback(
    std::function<void(const std::vector<float>&)> callback) {
    pimpl_->set_data_callback(callback);
}