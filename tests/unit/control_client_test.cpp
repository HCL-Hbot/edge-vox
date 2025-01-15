#include "../src/net/control_client.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

class EdgeVoxControlClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        client = std::make_unique<EdgeVoxControlClient>();
    }

    void TearDown() override {
        if (client && client->is_connected()) {
            client->disconnect();
        }
    }

    std::unique_ptr<EdgeVoxControlClient> client;
    std::vector<std::string> received_messages;

    // Helper function to create a status callback that stores messages
    void set_test_callback() {
        client->set_status_callback(
            [this](const std::string& status) { received_messages.push_back(status); });
    }

    // Helper to wait for connection/messages with timeout
    bool wait_for_condition(std::function<bool()> condition, int timeout_ms = 1000) {
        auto start = std::chrono::steady_clock::now();
        while (!condition()) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start)
                    .count() > timeout_ms) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }
};

TEST_F(EdgeVoxControlClientTest, InitializationTest) {
    EXPECT_FALSE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, ConnectionTest) {
    set_test_callback();
    EXPECT_TRUE(client->connect("localhost", 1883));

    // Wait for connection confirmation
    bool connected = wait_for_condition([this]() {
        return !received_messages.empty() &&
               received_messages.back().find("Connected") != std::string::npos;
    });

    EXPECT_TRUE(connected);
    EXPECT_TRUE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, DisconnectionTest) {
    ASSERT_TRUE(client->connect("localhost", 1883));
    ASSERT_TRUE(wait_for_condition([this]() { return client->is_connected(); }));

    client->disconnect();
    EXPECT_FALSE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, InvalidConnectionTest) {
    EXPECT_FALSE(client->connect("invalid_host", 1883));
    EXPECT_FALSE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, SendCommandTest) {
    set_test_callback();
    ASSERT_TRUE(client->connect("localhost", 1883));
    ASSERT_TRUE(wait_for_condition([this]() { return client->is_connected(); }));

    EXPECT_TRUE(client->send_command("test_command"));
}

TEST_F(EdgeVoxControlClientTest, SendCommandWhileDisconnectedTest) {
    EXPECT_FALSE(client->send_command("test_command"));
}

TEST_F(EdgeVoxControlClientTest, ReconnectionTest) {
    // Test multiple connect/disconnect cycles
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(client->connect("localhost", 1883));
        ASSERT_TRUE(wait_for_condition([this]() { return client->is_connected(); }));

        client->disconnect();
        EXPECT_FALSE(client->is_connected());

        // Small delay between cycles
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_F(EdgeVoxControlClientTest, MessageCallbackTest) {
    std::string test_message;
    client->set_status_callback(
        [&test_message](const std::string& status) { test_message = status; });

    ASSERT_TRUE(client->connect("localhost", 1883));
    ASSERT_TRUE(wait_for_condition([this]() { return client->is_connected(); }));

    // We should have received a connection message
    EXPECT_FALSE(test_message.empty());
}

TEST_F(EdgeVoxControlClientTest, UpdateCallbackTest) {
    std::string first_message;
    client->set_status_callback(
        [&first_message](const std::string& status) { first_message = status; });

    ASSERT_TRUE(client->connect("localhost", 1883));

    std::string second_message;
    client->set_status_callback(
        [&second_message](const std::string& status) { second_message = status; });

    // Send a command to trigger the new callback
    client->send_command("test_command");

    // First callback should not receive the message
    EXPECT_TRUE(first_message.empty() || first_message.find("Connected") != std::string::npos);
}
