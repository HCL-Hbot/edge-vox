#include "net/rtp_streamer.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

class EdgeVoxRtpStreamerTest : public ::testing::Test {
protected:
    void SetUp() override {
        streamer = std::make_unique<EdgeVoxRtpStreamer>();
        ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    }

    void TearDown() override {
        if (streamer && streamer->is_active()) {
            streamer->stop();
        }
    }

    std::unique_ptr<EdgeVoxRtpStreamer> streamer;

    // Helper method to create sample audio data
    std::vector<float> createTestSamples(size_t count) {
        std::vector<float> samples(count);
        for (size_t i = 0; i < count; i++) {
            samples[i] = static_cast<float>(i) / count;  // Creates a ramp from 0 to almost 1
        }
        return samples;
    }
};

TEST_F(EdgeVoxRtpStreamerTest, InitializationTest) {
    EdgeVoxRtpStreamer streamer;
    EXPECT_TRUE(streamer.init("127.0.0.1", 5004, 512));
}

TEST_F(EdgeVoxRtpStreamerTest, InitializationFailureTest) {
    EdgeVoxRtpStreamer streamer;
    // Test with various invalid addresses
    EXPECT_FALSE(streamer.init("256.256.256.256", 5004, 512));  // Invalid IP range
    EXPECT_FALSE(streamer.init("", 5004, 512));                 // Empty address
    EXPECT_FALSE(streamer.init("127.0.0", 5004, 512));          // Incomplete IP
}

TEST_F(EdgeVoxRtpStreamerTest, StartStopTest) {
    EXPECT_FALSE(streamer->is_active());
    EXPECT_TRUE(streamer->start());
    EXPECT_TRUE(streamer->is_active());
    streamer->stop();
    EXPECT_FALSE(streamer->is_active());
}

TEST_F(EdgeVoxRtpStreamerTest, DoubleStartTest) {
    EXPECT_TRUE(streamer->start());
    EXPECT_TRUE(streamer->start());  // Second start should still return true
    EXPECT_TRUE(streamer->is_active());
}

TEST_F(EdgeVoxRtpStreamerTest, SendAudioTest) {
    EXPECT_TRUE(streamer->start());
    auto samples = createTestSamples(256);
    EXPECT_TRUE(streamer->send_audio(samples));
}

TEST_F(EdgeVoxRtpStreamerTest, SendAudioWhileInactiveTest) {
    auto samples = createTestSamples(256);
    EXPECT_FALSE(streamer->send_audio(samples));  // Should fail when not started
}

TEST_F(EdgeVoxRtpStreamerTest, SendEmptyAudioTest) {
    EXPECT_TRUE(streamer->start());
    std::vector<float> empty_samples;
    EXPECT_TRUE(streamer->send_audio(empty_samples));  // Should handle empty samples gracefully
}

TEST_F(EdgeVoxRtpStreamerTest, SendLargeAudioTest) {
    EXPECT_TRUE(streamer->start());
    auto samples = createTestSamples(4096);  // Test with larger buffer
    EXPECT_TRUE(streamer->send_audio(samples));
}

TEST_F(EdgeVoxRtpStreamerTest, MultipleStartStopCyclesTest) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(streamer->start());
        EXPECT_TRUE(streamer->is_active());
        auto samples = createTestSamples(256);
        EXPECT_TRUE(streamer->send_audio(samples));
        streamer->stop();
        EXPECT_FALSE(streamer->is_active());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TEST_F(EdgeVoxRtpStreamerTest, DifferentPortsTest) {
    EdgeVoxRtpStreamer streamer1, streamer2;

    EXPECT_TRUE(streamer1.init("127.0.0.1", 5004, 512));
    EXPECT_TRUE(streamer2.init("127.0.0.1", 5005, 512));

    EXPECT_TRUE(streamer1.start());
    EXPECT_TRUE(streamer2.start());

    auto samples = createTestSamples(256);
    EXPECT_TRUE(streamer1.send_audio(samples));
    EXPECT_TRUE(streamer2.send_audio(samples));

    streamer1.stop();
    streamer2.stop();
}

TEST_F(EdgeVoxRtpStreamerTest, DifferentPayloadSizesTest) {
    EdgeVoxRtpStreamer small_streamer, large_streamer;

    EXPECT_TRUE(small_streamer.init("127.0.0.1", 5004, 256));   // Small payload
    EXPECT_TRUE(large_streamer.init("127.0.0.1", 5005, 1024));  // Large payload

    EXPECT_TRUE(small_streamer.start());
    EXPECT_TRUE(large_streamer.start());

    auto samples = createTestSamples(512);
    EXPECT_TRUE(small_streamer.send_audio(samples));
    EXPECT_TRUE(large_streamer.send_audio(samples));
}

// TEST_F(EdgeVoxRtpStreamerTest, BufferCapacityTest) {
//     ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
//     ASSERT_TRUE(streamer->start());

//     // Create large amount of samples
//     auto samples = createTestSamples(4800);  // 100ms of audio at 48kHz

//     // Keep sending until buffer is full
//     int packets_sent = 0;
//     while (!streamer->buffer_full() && packets_sent < 150) {  // Safety limit
//         EXPECT_TRUE(streamer->send_audio(samples));
//         packets_sent++;
//         // Small delay to allow send thread to process
//         std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     }

//     // Verify we could fill the buffer
//     EXPECT_TRUE(streamer->buffer_full());
//     EXPECT_GT(streamer->buffered_packets(), 0);
// }

// TEST_F(EdgeVoxRtpStreamerTest, BufferDrainTest) {
//     ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
//     ASSERT_TRUE(streamer->start());

//     // Fill buffer partially
//     auto samples = createTestSamples(480);  // 10ms of audio
//     for (int i = 0; i < 10; i++) {          // Send 100ms worth
//         EXPECT_TRUE(streamer->send_audio(samples));
//     }

//     // Wait for buffer to drain
//     auto start_time = std::chrono::steady_clock::now();
//     auto timeout = std::chrono::seconds(2);

//     while (streamer->buffered_packets() > 0) {
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         if (std::chrono::steady_clock::now() - start_time > timeout) {
//             FAIL() << "Buffer did not drain within timeout";
//             break;
//         }
//     }

//     EXPECT_EQ(streamer->buffered_packets(), 0);
// }

// TEST_F(EdgeVoxRtpStreamerTest, BufferClearOnStop) {
//     ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
//     ASSERT_TRUE(streamer->start());

//     // Fill buffer partially
//     auto samples = createTestSamples(480);
//     for (int i = 0; i < 5; i++) {
//         EXPECT_TRUE(streamer->send_audio(samples));
//     }

//     EXPECT_GT(streamer->buffered_packets(), 0);

//     streamer->stop();
//     EXPECT_EQ(streamer->buffered_packets(), 0);
// }

// TEST_F(EdgeVoxRtpStreamerTest, ContinuousStreamTest) {
//     ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
//     ASSERT_TRUE(streamer->start());

//     auto samples = createTestSamples(480);  // 10ms of audio
//     const int test_duration_ms = 500;       // Test for 500ms
//     const int sleep_interval_ms = 10;       // Sleep 10ms between sends

//     auto start_time = std::chrono::steady_clock::now();

//     while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()
//     -
//                                                                  start_time)
//                .count() < test_duration_ms) {
//         EXPECT_TRUE(streamer->send_audio(samples));
//         std::this_thread::sleep_for(std::chrono::milliseconds(sleep_interval_ms));
//     }

//     // Verify buffer isn't full (indicating packets are being sent)
//     EXPECT_FALSE(streamer->buffer_full());
// }

// TEST_F(EdgeVoxRtpStreamerTest, BackPressureTest) {
//     ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
//     ASSERT_TRUE(streamer->start());

//     // Try to overwhelm the buffer
//     auto samples = createTestSamples(4800);  // Large samples
//     bool backpressure_observed = false;

//     for (int i = 0; i < 200 && !backpressure_observed; i++) {
//         if (!streamer->send_audio(samples)) {
//             backpressure_observed = true;
//         }
//     }

//     EXPECT_TRUE(backpressure_observed) << "Expected back-pressure when buffer fills up";
// }