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

    std::vector<float> generate_test_audio(size_t samples, float frequency = 440.0f) {
        std::vector<float> audio(samples);
        for (size_t i = 0; i < samples; i++) {
            audio[i] = 0.5f * std::sin(2.0f * M_PI * frequency * i / AUDIO_SAMPLE_RATE);
        }
        return audio;
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

    if (!audio_buffer.empty()) {
        // Print first few samples if we got any
        std::cout << "First few samples: ";
        for (size_t i = 0; i < std::min(size_t(5), audio_buffer.size()); i++) {
            std::cout << audio_buffer[i] << " ";
        }
        std::cout << std::endl;

        // Calculate RMS
        double sum = 1e-10;  // Small epsilon
        for (const float sample : audio_buffer) {
            sum += static_cast<double>(sample * sample);
        }
        double rms = std::sqrt(sum / audio_buffer.size());
        std::cout << "\nRMS level: " << rms;
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

TEST_F(AudioAsyncTest, MultipleStartStopTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(audio->resume());
        // Clear any samples in queue
        EXPECT_TRUE(audio->clear());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_TRUE(audio->pause());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_F(AudioAsyncTest, BufferSizeTest) {
    const int buffer_ms = 1000;  // 1 second buffer
    auto audio_with_buffer = std::make_unique<audio_async>(buffer_ms);

    ASSERT_TRUE(audio_with_buffer->init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));
    ASSERT_TRUE(audio_with_buffer->resume());
    ASSERT_TRUE(audio_with_buffer->clear());

    // Wait for initial buffer fill
    std::this_thread::sleep_for(
        std::chrono::milliseconds(1200));  // Wait a bit longer than buffer_ms

    const size_t expected_samples = AUDIO_SAMPLE_RATE * (buffer_ms / 1000.0);
    std::vector<float> audio_data;
    bool success = false;
    const int MAX_ATTEMPTS = 10;

    for (int i = 0; i < MAX_ATTEMPTS && !success; i++) {
        audio_data.clear();
        audio_with_buffer->get(buffer_ms, audio_data);

        std::cout << "Got " << audio_data.size() << " samples on attempt " << (i + 1) << std::endl;

        // Check if we got enough samples
        if (audio_data.size() >= expected_samples * 0.9) {
            success = true;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    ASSERT_TRUE(success) << "Failed to get enough audio data after " << MAX_ATTEMPTS << " attempts";
    EXPECT_NEAR(audio_data.size(), expected_samples, expected_samples * 0.1);

    ASSERT_TRUE(audio_with_buffer->pause());
    ASSERT_TRUE(audio_with_buffer->close());
}

// Mock test to demonstrate how to test SDL callback functionality
class MockAudioAsync : public audio_async {
public:
    explicit MockAudioAsync(int len_ms) : audio_async(len_ms) {}

    // Expose protected methods for testing
    void test_capture_callback(uint8_t* stream, int len) {
        capture_callback(stream, len);
    }

    void test_playback_callback(uint8_t* stream, int len) {
        playback_callback(stream, len);
    }
};

TEST_F(AudioAsyncTest, CallbackTest) {
    MockAudioAsync mock_audio(1000);
    ASSERT_TRUE(mock_audio.init(AUDIO_DEFAULT_MIC, AUDIO_SAMPLE_RATE));

    // Start the audio capture
    ASSERT_TRUE(mock_audio.resume());

    // Create test audio data (one frame worth of samples)
    const int test_len = 1024 * sizeof(float);  // Match SDL buffer size
    std::vector<uint8_t> test_data(test_len);
    auto* float_ptr = reinterpret_cast<float*>(test_data.data());

    // Fill with simple sine wave
    const float frequency = 440.0f;  // A4 note
    for (int i = 0; i < 1024; ++i) {
        float t = static_cast<float>(i) / AUDIO_SAMPLE_RATE;
        float_ptr[i] = std::sin(2.0f * M_PI * frequency * t);
    }

    // Process multiple frames of test data
    const int num_frames = 5;
    for (int frame = 0; frame < num_frames; ++frame) {
        mock_audio.test_capture_callback(test_data.data(), test_len);
        // Small delay to simulate real-time behavior
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Verify the processed data
    std::vector<float> processed_data;
    mock_audio.get(100, processed_data);

    EXPECT_FALSE(processed_data.empty())
        << "No audio data received after processing " << num_frames << " frames";

    if (!processed_data.empty()) {
        // Verify we got a reasonable amount of data
        std::cout << "Received " << processed_data.size() << " samples" << std::endl;

        // Check that we got some non-zero samples
        bool has_nonzero = false;
        for (float sample : processed_data) {
            if (std::abs(sample) > 1e-6) {
                has_nonzero = true;
                break;
            }
        }
        EXPECT_TRUE(has_nonzero) << "All samples are zero";

        // Print first few samples for debugging
        std::cout << "First few samples: ";
        for (size_t i = 0; i < std::min(size_t(5), processed_data.size()); ++i) {
            std::cout << processed_data[i] << " ";
        }
        std::cout << std::endl;
    }

    ASSERT_TRUE(mock_audio.pause());
    ASSERT_TRUE(mock_audio.close());
}

TEST_F(AudioAsyncTest, PlaybackInitializationTest) {
    // Test initialization with both capture and playback
    EXPECT_TRUE(audio->init(AUDIO_DEFAULT_MIC, -1, AUDIO_SAMPLE_RATE));

    // Initially not playing
    EXPECT_FALSE(audio->is_playing());

    // Test if playback can be started
    EXPECT_TRUE(audio->start_playback());
    EXPECT_TRUE(audio->is_playing());

    // Add some test audio
    std::vector<float> test_audio = generate_test_audio(AUDIO_SAMPLE_RATE / 10);  // 100ms of audio
    EXPECT_TRUE(audio->play_audio(test_audio));

    // Let it play briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Test stopping playback
    EXPECT_TRUE(audio->stop_playback());
    EXPECT_FALSE(audio->is_playing());
}

TEST_F(AudioAsyncTest, AudioPlaybackTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, -1, AUDIO_SAMPLE_RATE));

    // Create test audio data (1 second of 440Hz sine wave)
    std::vector<float> test_audio(AUDIO_SAMPLE_RATE);
    for (size_t i = 0; i < test_audio.size(); i++) {
        test_audio[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / AUDIO_SAMPLE_RATE);
    }

    // Initially not playing
    EXPECT_FALSE(audio->is_playing());

    // Add audio to playback buffer
    EXPECT_TRUE(audio->play_audio(test_audio));

    // Start playback
    EXPECT_TRUE(audio->start_playback());
    EXPECT_TRUE(audio->is_playing());

    // Let it play for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop playback
    EXPECT_TRUE(audio->stop_playback());
    EXPECT_FALSE(audio->is_playing());
}

TEST_F(AudioAsyncTest, PlaybackBufferOverflowTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, -1, AUDIO_SAMPLE_RATE));

    // Initially not playing
    EXPECT_FALSE(audio->is_playing());

    // Create test audio data that's larger than the default buffer
    std::vector<float> large_audio(AUDIO_SAMPLE_RATE * 2);  // 2 seconds of audio
    for (size_t i = 0; i < large_audio.size(); i++) {
        large_audio[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / AUDIO_SAMPLE_RATE);
    }

    // Should still accept large amounts of audio
    EXPECT_TRUE(audio->play_audio(large_audio));

    // Start playback
    EXPECT_TRUE(audio->start_playback());
    EXPECT_TRUE(audio->is_playing());

    // Let it play briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop playback
    EXPECT_TRUE(audio->stop_playback());
    EXPECT_FALSE(audio->is_playing());
}

TEST_F(AudioAsyncTest, SimultaneousCapturePlaybackTest) {
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, -1, AUDIO_SAMPLE_RATE));

    // Start capturing
    ASSERT_TRUE(audio->resume());

    // Record for a short period
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Get the recorded audio
    std::vector<float> captured_audio;
    audio->get(100, captured_audio);  // Get 100ms of audio

    // Play it back
    EXPECT_TRUE(audio->play_audio(captured_audio));
    EXPECT_TRUE(audio->start_playback());

    // Let it play while still capturing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop everything
    EXPECT_TRUE(audio->stop_playback());
    EXPECT_TRUE(audio->pause());
}

TEST_F(AudioAsyncTest, InvalidPlaybackOperationsTest) {
    // Try playback operations before init
    EXPECT_FALSE(audio->start_playback());
    EXPECT_FALSE(audio->stop_playback());
    EXPECT_FALSE(audio->is_playing());

    std::vector<float> test_audio(1024, 0.0f);
    EXPECT_FALSE(audio->play_audio(test_audio));

    // Initialize only capture
    ASSERT_TRUE(audio->init(AUDIO_DEFAULT_MIC, -1, AUDIO_SAMPLE_RATE));

    // Now playback should work
    EXPECT_TRUE(audio->play_audio(test_audio));
    EXPECT_TRUE(audio->start_playback());
    EXPECT_TRUE(audio->stop_playback());
}
