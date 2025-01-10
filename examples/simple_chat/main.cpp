#include <atomic>
#include <csignal>
#include <edge_vox/core/client.hpp>
#include <iostream>
#include <thread>

static std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <port>\n";
        return 1;
    }

    // Setup signal handling
    signal(SIGINT, signal_handler);

    try {
        // Create client instance
        EdgeVoxClient client;

        // Configure audio settings
        EdgeVoxAudioConfig audio_cfg;
        audio_cfg.sample_rate = 48000;
        audio_cfg.channels = 1;
        audio_cfg.bits_per_sample = 16;
        audio_cfg.buffer_ms = 30;
        client.set_audio_config(audio_cfg);

        // Configure streaming settings
        EdgeVoxStreamConfig stream_cfg;
        stream_cfg.server_ip = argv[1];
        stream_cfg.rtp_port = std::stoi(argv[2]);
        stream_cfg.control_port = 1883;  // MQTT control port
        stream_cfg.packet_size = 512;
        client.set_stream_config(stream_cfg);

        // Set status callback
        client.set_status_callback(
            [](const std::string& status) { std::cout << "Status: " << status << std::endl; });

        // Connect to server
        if (!client.connect(argv[1], std::stoi(argv[2]))) {
            std::cerr << "Failed to connect to server\n";
            return 1;
        }

        // Start streaming
        if (!client.start_audio_stream()) {
            std::cerr << "Failed to start audio stream\n";
            return 1;
        }

        std::cout << "Streaming audio. Press Ctrl+C to stop...\n";

        // Main loop
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Cleanup
        client.stop_audio_stream();
        client.disconnect();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}