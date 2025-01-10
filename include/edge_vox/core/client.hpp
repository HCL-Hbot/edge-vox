#include <functional>
#include <memory>
#include <string>

#include "edge_vox/audio/audio_config.hpp"
#include "edge_vox/net/stream_config.hpp"

class EdgeVoxClient {
public:
    EdgeVoxClient();
    ~EdgeVoxClient();

    // Connection management
    bool connect(const std::string& server_ip, uint16_t port);
    void disconnect();
    bool is_connected() const;

    // Audio streaming
    bool start_audio_stream();
    void stop_audio_stream();
    bool is_streaming() const;

    // Configuration
    void set_audio_config(const EdgeVoxAudioConfig& config);
    void set_stream_config(const EdgeVoxStreamConfig& config);

    // Status callbacks
    using StatusCallback = std::function<void(const std::string& status)>;
    void set_status_callback(StatusCallback callback);

    // Event handlers
    using WakeWordCallback = std::function<void()>;
    void set_wake_word_callback(WakeWordCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};