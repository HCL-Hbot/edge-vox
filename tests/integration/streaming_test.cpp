#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>
#include <uvgrtp/lib.hh>

#include "net/rtp_receiver.hpp"
#include "net/rtp_streamer.hpp"

class EdgeVoxStreamingTest : public ::testing::Test {
protected:
    void SetUp() override {
        streamer = std::make_unique<EdgeVoxRtpStreamer>();
        receiver = std::make_unique<EdgeVoxRtpReceiver>();
    }

    void TearDown() override {
        if (streamer)
            streamer->stop();
        if (receiver)
            receiver->stop();
        received_samples.clear();
    }

    bool waitForData(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        auto start = std::chrono::steady_clock::now();
        while (!data_received) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    std::vector<float> generateSineWave(size_t samples, float frequency = 440.0f,
                                        float sampleRate = 48000.0f) {
        std::vector<float> audio(samples);
        for (size_t i = 0; i < samples; i++) {
            audio[i] = 0.5f * std::sin(2.0f * M_PI * frequency * i / sampleRate);
        }
        return audio;
    }

    std::unique_ptr<EdgeVoxRtpStreamer> streamer;
    std::unique_ptr<EdgeVoxRtpReceiver> receiver;
    std::vector<std::vector<float>> received_samples;
    std::atomic<bool> data_received{false};
    std::atomic<int> packet_count{0};
};

TEST_F(EdgeVoxStreamingTest, FullAudioPipelineTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(receiver->init("127.0.0.1", 5004));

    receiver->set_audio_callback([this](const std::vector<float>& samples) {
        received_samples.push_back(samples);
        data_received = true;
    });

    ASSERT_TRUE(receiver->start());
    ASSERT_TRUE(streamer->start());

    auto test_samples = generateSineWave(480);
    EXPECT_TRUE(streamer->send_audio(test_samples));
    EXPECT_TRUE(waitForData());
    EXPECT_FALSE(received_samples.empty());

    if (!received_samples.empty()) {
        EXPECT_EQ(received_samples[0].size(), test_samples.size());
        EXPECT_NEAR(received_samples[0][0], test_samples[0], 0.01f);
    }
}

TEST_F(EdgeVoxStreamingTest, ContinuousStreamingTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(receiver->init("127.0.0.1", 5004));

    receiver->set_audio_callback([this](const std::vector<float>& samples) { packet_count++; });

    ASSERT_TRUE(receiver->start());
    ASSERT_TRUE(streamer->start());

    const int num_packets = 50;
    auto test_samples = generateSineWave(480);

    for (int i = 0; i < num_packets; i++) {
        EXPECT_TRUE(streamer->send_audio(test_samples));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_GT(packet_count, num_packets / 2);
}

TEST_F(EdgeVoxStreamingTest, FrequencyStreamingTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(receiver->init("127.0.0.1", 5004));

    float test_freq = 440.0f;
    std::vector<float> received_sine;

    receiver->set_audio_callback([&](const std::vector<float>& samples) {
        received_sine = samples;
        data_received = true;
    });

    ASSERT_TRUE(receiver->start());
    ASSERT_TRUE(streamer->start());

    auto test_samples = generateSineWave(480, test_freq);
    EXPECT_TRUE(streamer->send_audio(test_samples));
    EXPECT_TRUE(waitForData());

    // Basic frequency content verification
    if (!received_sine.empty()) {
        int zero_crossings = 0;
        for (size_t i = 1; i < received_sine.size(); i++) {
            if ((received_sine[i - 1] < 0 && received_sine[i] >= 0) ||
                (received_sine[i - 1] >= 0 && received_sine[i] < 0)) {
                zero_crossings++;
            }
        }
        float expected_crossings = (test_freq * received_sine.size() / 48000.0f) * 2;
        EXPECT_NEAR(zero_crossings, expected_crossings, expected_crossings * 0.2f);
    }
}

TEST_F(EdgeVoxStreamingTest, NetworkStressTest) {
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 512));
    ASSERT_TRUE(receiver->init("127.0.0.1", 5004));

    std::atomic<int> dropped_packets{0};
    int last_packet_count = 0;

    receiver->set_audio_callback([this](const std::vector<float>& samples) { packet_count++; });

    ASSERT_TRUE(receiver->start());
    ASSERT_TRUE(streamer->start());

    const int total_packets = 1000;
    const int burst_size = 10;
    auto test_samples = generateSineWave(480);

    for (int i = 0; i < total_packets / burst_size; i++) {
        // Send burst of packets
        for (int j = 0; j < burst_size; j++) {
            EXPECT_TRUE(streamer->send_audio(test_samples));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_GT(packet_count, total_packets * 0.8);  // Allow up to 20% packet loss
}

TEST_F(EdgeVoxStreamingTest, LargePacketTest) {
    int sender_flags = RCE_FRAGMENT_GENERIC | RCE_SEND_ONLY;
    int receiver_flags = RCE_FRAGMENT_GENERIC | RCE_RECEIVE_ONLY;
    ASSERT_TRUE(streamer->init("127.0.0.1", 5004, 2048, sender_flags));
    ASSERT_TRUE(receiver->init("127.0.0.1", 5004, receiver_flags));

    receiver->set_audio_callback([this](const std::vector<float>& samples) {
        received_samples.push_back(samples);
        data_received = true;
    });

    ASSERT_TRUE(receiver->start());
    ASSERT_TRUE(streamer->start());

    auto test_samples = generateSineWave(1920);  // 40ms of audio at 48kHz
    EXPECT_TRUE(streamer->send_audio(test_samples));
    EXPECT_TRUE(waitForData());
    EXPECT_FALSE(received_samples.empty());

    if (!received_samples.empty()) {
        EXPECT_EQ(received_samples[0].size(), test_samples.size());
    }
}