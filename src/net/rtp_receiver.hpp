#pragma once
#include <functional>
#include <memory>
#include <string>
#include <uvgrtp/lib.hh>

class EdgeVoxRtpReceiver {
public:
    EdgeVoxRtpReceiver();
    ~EdgeVoxRtpReceiver();

    using AudioCallback = std::function<void(const std::vector<float>&)>;

    bool init(const std::string& local_ip, uint16_t port, int flags = RCE_RECEIVE_ONLY);

    bool start();
    void stop();
    bool is_active() const;
    void set_audio_callback(AudioCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};