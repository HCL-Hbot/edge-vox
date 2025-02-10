#include "net/rtp_receiver.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <uvgrtp/lib.hh>

class EdgeVoxRtpReceiverTest : public ::testing::Test {
protected:
    void SetUp() override {
        receiver = std::make_unique<EdgeVoxRtpReceiver>();
    }

    void TearDown() override {
        if (receiver) {
            receiver->stop();
            receiver.reset();
        }
    }

    std::unique_ptr<EdgeVoxRtpReceiver> receiver;
    std::vector<std::vector<float>> received_samples;

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

TEST_F(EdgeVoxRtpReceiverTest, InitializationTest) {
    EXPECT_FALSE(receiver->is_active());
    EXPECT_TRUE(receiver->init("127.0.0.1", 5005));
}

TEST_F(EdgeVoxRtpReceiverTest, InvalidInitializationTest) {
    EXPECT_FALSE(receiver->init("invalid_ip", 5005));
    EXPECT_FALSE(receiver->init("127.0.0.1", 0));
}

TEST_F(EdgeVoxRtpReceiverTest, StartStopTest) {
    ASSERT_TRUE(receiver->init("127.0.0.1", 5005));

    EXPECT_FALSE(receiver->is_active());
    EXPECT_TRUE(receiver->start());
    EXPECT_TRUE(receiver->is_active());

    receiver->stop();
    EXPECT_FALSE(receiver->is_active());
}

TEST_F(EdgeVoxRtpReceiverTest, CallbackTest) {
    ASSERT_TRUE(receiver->init("127.0.0.1", 5005));

    bool callback_called = false;
    receiver->set_audio_callback([&](const std::vector<float>& samples) {
        callback_called = true;
        received_samples.push_back(samples);
    });

    ASSERT_TRUE(receiver->start());

    // Create a sender to test callback
    uvgrtp::context ctx;
    auto* sess = ctx.create_session("127.0.0.1");
    ASSERT_TRUE(sess != nullptr);

    auto* stream = sess->create_stream(5005, RTP_FORMAT_GENERIC, RCE_SEND_ONLY);
    ASSERT_TRUE(stream != nullptr);

    // Send test audio data
    std::vector<int16_t> test_data(480, 16384);  // 10ms of audio at 48kHz
    EXPECT_TRUE(stream->push_frame(reinterpret_cast<uint8_t*>(test_data.data()),
                                   test_data.size() * sizeof(int16_t), RTP_NO_FLAGS) == RTP_OK);

    // Wait for callback
    EXPECT_TRUE(waitForCondition([&]() { return callback_called; }));
    EXPECT_FALSE(received_samples.empty());

    if (!received_samples.empty()) {
        EXPECT_EQ(received_samples[0].size(), test_data.size());
        // Check conversion from int16 to float
        EXPECT_NEAR(received_samples[0][0], test_data[0] / 32767.0f, 0.0001f);
    }

    // Cleanup sender
    sess->destroy_stream(stream);
    ctx.destroy_session(sess);
}

TEST_F(EdgeVoxRtpReceiverTest, MultipleStartStopCyclesTest) {
    ASSERT_TRUE(receiver->init("127.0.0.1", 5005));

    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(receiver->start()) << "Failed to start on iteration " << i;
        EXPECT_TRUE(receiver->is_active());

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        receiver->stop();
        EXPECT_FALSE(receiver->is_active());

        // Allow time for cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Re-init for next cycle
        if (i < 2) {
            ASSERT_TRUE(receiver->init("127.0.0.1", 5005));
        }
    }
}

TEST_F(EdgeVoxRtpReceiverTest, CallbackUpdateTest) {
    ASSERT_TRUE(receiver->init("127.0.0.1", 5005));

    std::atomic<int> first_callback_count{0};
    receiver->set_audio_callback(
        [&](const std::vector<float>& samples) { first_callback_count++; });

    std::atomic<int> second_callback_count{0};
    receiver->set_audio_callback(
        [&](const std::vector<float>& samples) { second_callback_count++; });

    ASSERT_TRUE(receiver->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(first_callback_count, 0);
    receiver->stop();
}

TEST_F(EdgeVoxRtpReceiverTest, StopWithoutStartTest) {
    ASSERT_TRUE(receiver->init("127.0.0.1", 5005));
    receiver->stop();  // Should not crash
    EXPECT_FALSE(receiver->is_active());
}
