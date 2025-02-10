#include "net/rtp_streamer.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>

class EdgeVoxRtpStreamerTest : public ::testing::Test {
protected:
    void SetUp() override {
        streamer = std::make_unique<EdgeVoxRtpStreamer>();
    }

    void TearDown() override {
        if (streamer && streamer->is_active()) {
            streamer->stop();
        }
    }

    std::unique_ptr<EdgeVoxRtpStreamer> streamer;

    // Helper method to create test audio samples
    std::vector<float> createTestSamples(size_t count) {
        std::vector<float> samples(count);
        for (size_t i = 0; i < count; i++) {
            // Create a simple sine wave
            samples[i] = 0.5f * std::sin(2.0f * M_PI * i / count);
        }
        return samples;
    }

    // Helper method to wait for a condition with timeout
    bool waitForCondition(std::function<bool()> condition,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        auto start = std::chrono::steady_clock::now();
        while (!condition()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }
};

TEST_F(EdgeVoxRtpStreamerTest, InitializationTest) {
    EXPECT_FALSE(streamer->is_active());
    EXPECT_TRUE(streamer->init("127.0.0.1", 5004, 512));
}

TEST_F(EdgeVoxRtpStreamerTest, InvalidInitializationTest) {
    // Test with invalid IP
    EXPECT_FALSE(streamer->init("invalid_ip", 5004, 512));

    // Test with invalid port
    EXPECT_FALSE(streamer->init("127.0.0.1", 0, 512));

    // Test with invalid payload size
    EXPECT_FALSE(streamer->init("127.0.0.1", 5004, 0));
}

TEST_F(EdgeVoxRtpStreamerTest, StartStopTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));

    EXPECT_FALSE(streamer->is_active());
    EXPECT_TRUE(streamer->start());
    EXPECT_TRUE(streamer->is_active());

    streamer->stop();
    EXPECT_FALSE(streamer->is_active());
}

TEST_F(EdgeVoxRtpStreamerTest, DoubleStartTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));

    EXPECT_TRUE(streamer->start());
    EXPECT_TRUE(streamer->start());  // Second start should still return true
    EXPECT_TRUE(streamer->is_active());
}

TEST_F(EdgeVoxRtpStreamerTest, SendAudioTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(streamer->start());

    auto samples = createTestSamples(480);  // 10ms at 48kHz
    EXPECT_TRUE(streamer->send_audio(samples));
}

TEST_F(EdgeVoxRtpStreamerTest, SendWithoutStartTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));

    auto samples = createTestSamples(480);
    EXPECT_FALSE(streamer->send_audio(samples));  // Should fail when not started
}

TEST_F(EdgeVoxRtpStreamerTest, SendEmptyAudioTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(streamer->start());

    std::vector<float> empty_samples;
    EXPECT_FALSE(streamer->send_audio(empty_samples));  // Should handle empty samples gracefully
}

TEST_F(EdgeVoxRtpStreamerTest, MultipleStartStopCyclesTest) {
    for (int i = 0; i < 5; ++i) {
        std::cout << "Cycle " << i + 1 << "/5" << std::endl;

        // Re-initialize for each cycle
        ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
        EXPECT_TRUE(streamer->start());
        EXPECT_TRUE(streamer->is_active());

        auto samples = createTestSamples(480);
        EXPECT_TRUE(streamer->send_audio(samples));

        streamer->stop();
        EXPECT_FALSE(streamer->is_active());

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

TEST_F(EdgeVoxRtpStreamerTest, DifferentPortsTest) {
    EdgeVoxRtpStreamer streamer1, streamer2;

    EXPECT_TRUE(streamer1.init("127.0.0.1", 5004, 512));
    EXPECT_TRUE(streamer2.init("127.0.0.1", 5005, 512));

    EXPECT_TRUE(streamer1.start());
    EXPECT_TRUE(streamer2.start());

    auto samples = createTestSamples(480);
    EXPECT_TRUE(streamer1.send_audio(samples));
    EXPECT_TRUE(streamer2.send_audio(samples));
}

TEST_F(EdgeVoxRtpStreamerTest, StressTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(streamer->start());

    const int numPackets = 100;
    const size_t sampleSize = 480;
    auto samples = createTestSamples(sampleSize);

    int successCount = 0;
    for (int i = 0; i < numPackets; ++i) {
        if (streamer->send_audio(samples)) {
            successCount++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // We expect most packets to be sent successfully
    EXPECT_GT(successCount, numPackets * 0.9);
}

TEST_F(EdgeVoxRtpStreamerTest, CleanupTest) {
    // Create a scope for the streamer
    {
        EdgeVoxRtpStreamer localStreamer;
        ASSERT_TRUE(localStreamer.init("127.0.0.1", 5004, 512));
        ASSERT_TRUE(localStreamer.start());

        auto samples = createTestSamples(480);
        EXPECT_TRUE(localStreamer.send_audio(samples));

        // Let localStreamer go out of scope without explicit cleanup
    }

    // Create a new streamer with the same settings
    EdgeVoxRtpStreamer newStreamer;
    EXPECT_TRUE(newStreamer.init("127.0.0.1", 5004, 512));
    EXPECT_TRUE(newStreamer.start());
}