// tests/unit/audio_async_test.cpp
#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <edge_vox/audio/audio_async.hpp>
#include <thread>

#define AUDIO_DEFAULT_MIC -1
#define AUDIO_SAMPLE_RATE 16000

class AudioAsyncTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        // Ensure SDL is not initialized at start
        while (SDL_WasInit(SDL_INIT_AUDIO)) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        SDL_Quit();
    }

    static void TearDownTestSuite() {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        // Add a small delay to let SDL fully exit
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        SDL_Quit();
        // Add a small delay to let SDL fully exit
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void SetUp() override {
        audio = std::make_shared<audio_async>(5 * 1000);
    }

    void TearDown() override {
        if (audio) {
            audio->pause();
            audio.reset();  // Force destruction before SDL cleanup
        }
    }

    std::shared_ptr<audio_async> audio;
};

TEST_F(AudioAsyncTest, InitializationTest) {
    // Test initialization with default device (-1) and standard sample rate
    EXPECT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

    // Properly quit SDL
    EXPECT_TRUE(audio->close());

    // Add a small delay to let SDL fully exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(AudioAsyncTest, StartStopTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

    // Test starting audio capture
    EXPECT_TRUE(audio->resume());

    // Clear any samples in queue
    EXPECT_TRUE(audio->clear());

    // Wait a small amount to ensure audio system is running
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Test stopping audio capture
    EXPECT_TRUE(audio->pause());

    // Properly quit SDL
    EXPECT_TRUE(audio->close());

    // Add a small delay to let SDL fully exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Add a small delay to let SDL fully exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(AudioAsyncTest, BufferClearTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

    // Add a small delay after init to let SDL fully set up
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    ASSERT_TRUE(audio->resume());

    // Wait to collect some audio data
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Test clearing the buffer
    EXPECT_TRUE(audio->clear());

    std::vector<float> audio_data;
    audio->get(50, audio_data);  // Get 50ms of audio

    // After clearing, we should have very little or no data
    EXPECT_TRUE(audio_data.empty() ||
                audio_data.size() < AUDIO_SAMPLE_RATE * 0.05);  // 0.05 seconds worth of samples

    // Properly quit SDL
    EXPECT_TRUE(audio->close());

    // Add a small delay to let SDL fully exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(AudioAsyncTest, BasicAudioCaptureTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));
    ASSERT_TRUE(audio->resume());
    ASSERT_TRUE(audio->clear());

    std::vector<float> audio_buffer;
    audio_buffer.reserve(2560);

    const int MAX_ATTEMPTS = 10;
    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        audio_buffer.clear();
        audio->get(1000, audio_buffer);

        std::cout << "Attempt " << (i + 1) << ": got " << audio_buffer.size() << " samples"
                  << std::endl;

        if (!audio_buffer.empty()) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ASSERT_FALSE(audio_buffer.empty())
        << "Failed to capture any audio data after " << MAX_ATTEMPTS << " attempts";

    // Print first few samples if we got any
    if (!audio_buffer.empty()) {
        std::cout << "First few samples: ";
        for (size_t i = 0; i < std::min(size_t(5), audio_buffer.size()); i++) {
            std::cout << audio_buffer[i] << " ";
        }
        std::cout << std::endl;
        audio_buffer.clear();
    }

    // Properly quit SDL
    EXPECT_TRUE(audio->close());

    // Add a small delay to let SDL fully exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

TEST_F(AudioAsyncTest, AudioDataValidityTest) {
    // Initialize audio device
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

    // Start audio capture
    ASSERT_TRUE(audio->resume());

    // Clear any samples in queue
    ASSERT_TRUE(audio->clear());

    // Create audio buffer and reserve space
    std::vector<float> audio_buffer;
    audio_buffer.reserve(2560);

    // Variables for test control
    bool got_data = false;
    int attempts = 0;
    const int MAX_ATTEMPTS = 5;

    // Main capture loop, similar to working code
    while (!got_data && attempts < MAX_ATTEMPTS) {
        audio->get(1000, audio_buffer);

        if (!audio_buffer.empty()) {
            got_data = true;
            std::cout << "Got " << audio_buffer.size() << " samples on attempt " << (attempts + 1)
                      << std::endl;
        } else {
            std::cout << "No data on attempt " << (attempts + 1) << std::endl;
            audio_buffer.clear();
            attempts++;
        }

        // Same sleep as working code
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    ASSERT_TRUE(got_data) << "Failed to get audio data after " << MAX_ATTEMPTS << " attempts";
    ASSERT_FALSE(audio_buffer.empty());

    // Verify data
    bool has_nonzero = false;
    for (float sample : audio_buffer) {
        EXPECT_GE(sample, -1.0f);
        EXPECT_LE(sample, 1.0f);
        if (sample != 0.0f) {
            has_nonzero = true;
        }
    }

    EXPECT_TRUE(has_nonzero) << "All samples are zero - might not be capturing actual audio";

    EXPECT_TRUE(audio->close());

    // Add a small delay to let SDL fully exit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

// TEST_F(AudioAsyncTest, MultipleStartStopTest) {
//     ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

//     for (int i = 0; i < 3; ++i) {
//         EXPECT_TRUE(audio->resume());
//         // Clear any samples in queue
//         // EXPECT_TRUE(audio->clear());
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         EXPECT_TRUE(audio->pause());
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }

// TEST_F(AudioAsyncTest, BufferSizeTest) {
//     const int buffer_ms = 1000;  // 1 second buffer
//     auto audio_with_buffer = std::make_unique<audio_async>(buffer_ms);

//     ASSERT_TRUE(audio_with_buffer->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));
//     std::this_thread::sleep_for(std::chrono::milliseconds(500));
//     ASSERT_TRUE(audio_with_buffer->resume());

//     // Record for longer than buffer size
//     std::this_thread::sleep_for(std::chrono::milliseconds(buffer_ms + 500));

//     std::vector<float> audio_data;
//     audio_with_buffer->get(buffer_ms + 500, audio_data);

//     // Should only get buffer_ms worth of data
//     const size_t expected_samples = AUDIO_SAMPLE_RATE * (buffer_ms / 1000.0);
//     EXPECT_NEAR(audio_data.size(), expected_samples, expected_samples * 0.1);
// }

// TEST_F(AudioAsyncTest, InvalidOperationsTest) {
//     // Try to pause before init
//     EXPECT_FALSE(audio->pause());

//     // Try to get data before init
//     std::vector<float> audio_data;
//     audio->get(100, audio_data);
//     EXPECT_TRUE(audio_data.empty());

//     // Initialize with invalid sample rate
//     EXPECT_FALSE(audio->init(-1, 0));
// }

// // Mock test to demonstrate how to test SDL callback functionality
// class MockAudioAsync : public audio_async {
// public:
//     explicit MockAudioAsync(int len_ms) : audio_async(len_ms) {}

//     // Expose protected methods for testing
//     void test_callback(uint8_t* stream, int len) {
//         callback(stream, len);
//     }
// };

// TEST_F(AudioAsyncTest, CallbackTest) {
//     MockAudioAsync mock_audio(1000);
//     ASSERT_TRUE(mock_audio.init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

//     // Create test audio data
//     const int test_len = 1024;
//     std::vector<uint8_t> test_data(test_len);

//     // Fill with simple sine wave
//     for (int i = 0; i < test_len / 4; ++i) {
//         float sample = std::sin(2.0f * M_PI * 440.0f * i / float(AUDIO_SAMPLE_RATE));
//         auto* float_ptr = reinterpret_cast<float*>(test_data.data());
//         float_ptr[i] = sample;
//     }

//     // Process the test data through SDL callback
//     mock_audio.test_callback(test_data.data(), test_len);

//     // Verify the processed data
//     std::vector<float> processed_data;
//     mock_audio.get(100, processed_data);

//     EXPECT_FALSE(processed_data.empty());
// }