// seems to run allright

// ./edge-vox/tests/audio_reproducer/build/audio_reproducer

// init: found 1 capture devices:
// init:    - Capture device #0: 'Built-in Audio Analog Stereo'
// init: attempt to open default capture device ...
// init: obtained spec for input device (SDL Id = 2):
// init:     - sample rate:       16000
// init:     - format:            33056 (required: 33056)
// init:     - channels:          1 (required: 1)
// init:     - samples per frame: 1024
// Attempt 1
// Got 0 samples
// Attempt 2
// Got 2048 samples
// First few samples: -1.11506 -1.8967 -1.66577 -1.76528 -1.73687
// Attempt 3
// Got 5120 samples
// First few samples: -1.11506 -1.8967 -1.66577 -1.76528 -1.73687
// Attempt 4
// Got 8192 samples
// First few samples: -1.11506 -1.8967 -1.66577 -1.76528 -1.73687
// Attempt 5
// Got 11264 samples
// First few samples: -1.11506 -1.8967 -1.66577 -1.76528 -1.73687
// Attempt 6
// Got 14336 samples
// First few samples: -1.11506 -1.8967 -1.66577 -1.76528 -1.73687
// Attempt 7
// Got 16000 samples
// First few samples: 0.865095 0.864778 0.865968 0.864643 0.861652
// Attempt 8
// Got 16000 samples
// First few samples: 0.237541 0.234269 0.233906 0.236894 0.235017
// Attempt 9
// Got 16000 samples
// First few samples: 0.0403077 0.0424719 0.0393424 0.0377386 0.0408216
// Attempt 10
// Got 16000 samples
// First few samples: -0.0189629 -0.0176605 -0.0170245 -0.0177422 -0.0140201
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "edge_vox/audio/audio_async.hpp"

int main() {
    // Create audio object exactly like the working code
    std::shared_ptr<audio_async> audio = std::make_shared<audio_async>(5 * 1000);

    // Initialize
    if (!audio->init(-1, 16000)) {
        std::cerr << "Failed to init audio" << std::endl;
        return 1;
    }

    // Start audio
    if (!audio->resume()) {
        std::cerr << "Failed to resume audio" << std::endl;
        return 1;
    }

    // Clear buffer
    if (!audio->clear()) {
        std::cerr << "Failed to clear audio buffer" << std::endl;
        return 1;
    }

    // Create audio buffer
    std::vector<float> audio_buffer;
    audio_buffer.reserve(2560);

    // Try to get audio data several times
    for (int i = 0; i < 10; i++) {
        std::cout << "Attempt " << (i + 1) << std::endl;

        audio_buffer.clear();
        audio->get(1000, audio_buffer);

        std::cout << "Got " << audio_buffer.size() << " samples" << std::endl;

        if (!audio_buffer.empty()) {
            std::cout << "First few samples: ";
            for (size_t j = 0; j < std::min(size_t(5), audio_buffer.size()); j++) {
                std::cout << audio_buffer[j] << " ";
            }
            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    audio->pause();
    return 0;
}