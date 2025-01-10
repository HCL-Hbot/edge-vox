#pragma once
#include <memory>
#include <string>
#include <vector>

class EdgeVoxRtpStreamer {
public:
    EdgeVoxRtpStreamer();
    ~EdgeVoxRtpStreamer();

    bool init(const std::string& host, uint16_t port, uint32_t payload_size);
    bool start();
    void stop();
    bool send_audio(const std::vector<float>& samples);
    bool is_active() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};