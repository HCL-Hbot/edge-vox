#include "net/packet_buffer.hpp"

#include <gtest/gtest.h>

#include <random>
#include <thread>

class PacketBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<PacketBuffer>(10);  // Use smaller size for testing
    }

    std::vector<uint8_t> createTestPacket(size_t size, uint8_t value) {
        return std::vector<uint8_t>(size, value);
    }

    std::unique_ptr<PacketBuffer> buffer;
};

TEST_F(PacketBufferTest, BasicOperations) {
    auto packet1 = createTestPacket(10, 1);
    auto packet2 = createTestPacket(10, 2);
    std::vector<uint8_t> outPacket;

    EXPECT_TRUE(buffer->empty());
    EXPECT_FALSE(buffer->full());

    EXPECT_TRUE(buffer->push(packet1));
    EXPECT_FALSE(buffer->empty());

    EXPECT_TRUE(buffer->pop(outPacket));
    EXPECT_EQ(outPacket, packet1);
    EXPECT_TRUE(buffer->empty());
}

TEST_F(PacketBufferTest, FullBufferBehavior) {
    for (int i = 0; i < 10; i++) {
        EXPECT_TRUE(buffer->push(createTestPacket(10, i)));
    }

    EXPECT_TRUE(buffer->full());
    EXPECT_FALSE(buffer->push(createTestPacket(10, 99)));  // Should fail
}

TEST_F(PacketBufferTest, CircularBehavior) {
    // Fill buffer
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(buffer->push(createTestPacket(10, i)));
    }

    // Remove some packets
    std::vector<uint8_t> outPacket;
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(buffer->pop(outPacket));
        EXPECT_EQ(outPacket[0], i);
    }

    // Add more packets
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(buffer->push(createTestPacket(10, i + 10)));
    }

    // Verify FIFO order
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(buffer->pop(outPacket));
        if (i < 2) {
            EXPECT_EQ(outPacket[0], i + 3);
        } else {
            EXPECT_EQ(outPacket[0], i + 8);
        }
    }
}

TEST_F(PacketBufferTest, PeekOperation) {
    auto packet = createTestPacket(10, 42);
    std::vector<uint8_t> peekPacket;

    EXPECT_TRUE(buffer->push(packet));
    EXPECT_TRUE(buffer->peek(peekPacket));
    EXPECT_EQ(peekPacket, packet);
    EXPECT_FALSE(buffer->empty());  // Peek shouldn't remove packet
}

TEST_F(PacketBufferTest, ClearOperation) {
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(buffer->push(createTestPacket(10, i)));
    }

    EXPECT_FALSE(buffer->empty());
    buffer->clear();
    EXPECT_TRUE(buffer->empty());
    EXPECT_EQ(buffer->size(), 0);
}

TEST_F(PacketBufferTest, ThreadSafety) {
    const int numIterations = 100;
    std::atomic<bool> shouldStop{false};
    std::atomic<int> pushCount{0};
    std::atomic<int> popCount{0};

    // Consumer thread - start this first to be ready for data
    auto consumer = std::thread([&]() {
        std::vector<uint8_t> packet;
        while (!shouldStop && popCount < numIterations) {
            if (buffer->pop(packet)) {
                popCount++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Small delay to ensure consumer is ready
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Producer thread
    auto producer = std::thread([&]() {
        for (int i = 0; i < numIterations && !shouldStop; i++) {
            auto testPacket = createTestPacket(10, i % 255);
            // Keep trying to push until successful
            while (!shouldStop && !buffer->push(testPacket)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            if (!shouldStop) {
                pushCount++;
                // Print progress every 10 packets
                if (i % 10 == 0) {
                    std::cout << "Pushed " << pushCount << " packets\n";
                }
            }
        }
    });

    // Monitoring loop with timeout
    auto start = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(5);

    while (std::chrono::steady_clock::now() - start < timeout) {
        if (pushCount >= numIterations && popCount >= numIterations) {
            break;  // Success case - completed all iterations
        }

        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(4)) {
            std::cout << "Debug - Push count: " << pushCount << ", Pop count: " << popCount
                      << ", Buffer size: " << buffer->size() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    shouldStop = true;  // Signal threads to stop

    producer.join();
    consumer.join();

    std::cout << "Final state - Push count: " << pushCount << ", Pop count: " << popCount
              << ", Buffer size: " << buffer->size() << std::endl;

    EXPECT_EQ(pushCount.load(), numIterations) << "Failed to push expected number of packets";
    EXPECT_EQ(popCount.load(), numIterations) << "Failed to pop expected number of packets";
    EXPECT_TRUE(buffer->empty()) << "Buffer not empty, size: " << buffer->size();
}