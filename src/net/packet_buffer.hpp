#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <vector>

class PacketBuffer {
public:
    // Default to 1 second of audio at 48kHz with 10ms packets
    PacketBuffer(size_t capacity = 100)
        : capacity_(capacity), write_pos_(0), read_pos_(0), packet_count_(0) {
        buffer_.resize(capacity);
    }

    bool push(const std::vector<uint8_t>& packet) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (packet_count_ >= capacity_) {
            return false;  // Buffer full
        }

        buffer_[write_pos_] = packet;
        write_pos_ = (write_pos_ + 1) % capacity_;
        packet_count_++;

        return true;
    }

    bool pop(std::vector<uint8_t>& packet) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (packet_count_ == 0) {
            return false;  // Buffer empty
        }

        packet = std::move(buffer_[read_pos_]);
        read_pos_ = (read_pos_ + 1) % capacity_;
        packet_count_--;

        return true;
    }

    bool peek(std::vector<uint8_t>& packet) const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (packet_count_ == 0) {
            return false;
        }

        packet = buffer_[read_pos_];
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return packet_count_;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return packet_count_ == 0;
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return packet_count_ >= capacity_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        write_pos_ = 0;
        read_pos_ = 0;
        packet_count_ = 0;
        buffer_.clear();
        buffer_.resize(capacity_);
    }

private:
    const size_t capacity_;
    size_t write_pos_;
    size_t read_pos_;
    size_t packet_count_;
    std::vector<std::vector<uint8_t>> buffer_;
    mutable std::mutex mutex_;
};