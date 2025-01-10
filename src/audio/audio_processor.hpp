#pragma once
#include <functional>
#include <memory>
#include <vector>

class EdgeVoxAudioProcessor {
public:
    EdgeVoxAudioProcessor();
    ~EdgeVoxAudioProcessor();

    bool init(int device_id, uint32_t sample_rate);
    void start();
    void stop();
    void get_samples(std::vector<float>& samples);
    void set_data_callback(std::function<void(const std::vector<float>&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};