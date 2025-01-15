#include "../src/net/control_client.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mosquitto.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

// Store callback to trigger it later
void (*stored_connect_callback)(struct mosquitto*, void*, int);
void (*stored_message_callback)(struct mosquitto*, void*, const struct mosquitto_message*);
void* stored_userdata = nullptr;

// Mock class for Mosquitto
class MockMosquitto {
public:
    MOCK_METHOD(int, connect, (const char* host, int port, int keepalive));
    MOCK_METHOD(int, disconnect, ());
    MOCK_METHOD(int, loop_start, ());
    MOCK_METHOD(int, loop_stop, (bool force));
    MOCK_METHOD(int, publish,
                (int* mid, const char* topic, int payloadlen, const void* payload, int qos,
                 bool retain));
    MOCK_METHOD(void, message_callback_set,
                (void (*callback)(struct mosquitto*, void*, const struct mosquitto_message*)));
    MOCK_METHOD(void, connect_callback_set, (void (*callback)(struct mosquitto*, void*, int)));

    void trigger_connect_callback(struct mosquitto* mosq, int rc) {
        if (stored_connect_callback) {
            stored_connect_callback(mosq, stored_userdata, rc);
        }
    }

    void trigger_message_callback(struct mosquitto* mosq, const struct mosquitto_message* message) {
        if (stored_message_callback) {
            stored_message_callback(mosq, stored_userdata, message);
        }
    }

    static MockMosquitto* instance() {
        static MockMosquitto instance;
        return &instance;
    }
};

// Mock functions to replace Mosquitto C API
extern "C" {
struct mosquitto* mosquitto_new(const char* id, bool clean_session, void* obj) {
    stored_userdata = obj;
    return reinterpret_cast<struct mosquitto*>(MockMosquitto::instance());
}

int mosquitto_connect(struct mosquitto* mosq, const char* host, int port, int keepalive) {
    return reinterpret_cast<MockMosquitto*>(mosq)->connect(host, port, keepalive);
}

int mosquitto_disconnect(struct mosquitto* mosq) {
    return reinterpret_cast<MockMosquitto*>(mosq)->disconnect();
}

int mosquitto_loop_start(struct mosquitto* mosq) {
    return reinterpret_cast<MockMosquitto*>(mosq)->loop_start();
}

int mosquitto_loop_stop(struct mosquitto* mosq, bool force) {
    return reinterpret_cast<MockMosquitto*>(mosq)->loop_stop(force);
}

int mosquitto_publish(struct mosquitto* mosq, int* mid, const char* topic, int payloadlen,
                      const void* payload, int qos, bool retain) {
    return reinterpret_cast<MockMosquitto*>(mosq)->publish(mid, topic, payloadlen, payload, qos,
                                                           retain);
}

void mosquitto_message_callback_set(struct mosquitto* mosq,
                                    void (*callback)(struct mosquitto*, void*,
                                                     const struct mosquitto_message*)) {
    stored_message_callback = callback;
    reinterpret_cast<MockMosquitto*>(mosq)->message_callback_set(callback);
}

void mosquitto_connect_callback_set(struct mosquitto* mosq,
                                    void (*callback)(struct mosquitto*, void*, int)) {
    stored_connect_callback = callback;
    reinterpret_cast<MockMosquitto*>(mosq)->connect_callback_set(callback);
}

void mosquitto_destroy(struct mosquitto* mosq) {}
int mosquitto_lib_init() {
    return MOSQ_ERR_SUCCESS;
}
int mosquitto_lib_cleanup() {
    return MOSQ_ERR_SUCCESS;
}
}

class EdgeVoxControlClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_mosquitto = MockMosquitto::instance();

        // Set default expectations for common calls
        using ::testing::_;
        using ::testing::Return;
        EXPECT_CALL(*mock_mosquitto, message_callback_set(_)).Times(::testing::AnyNumber());
        EXPECT_CALL(*mock_mosquitto, connect_callback_set(_)).Times(::testing::AnyNumber());
        EXPECT_CALL(*mock_mosquitto, disconnect())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(Return(MOSQ_ERR_SUCCESS));
        EXPECT_CALL(*mock_mosquitto, loop_stop(_))
            .Times(::testing::AnyNumber())
            .WillRepeatedly(Return(MOSQ_ERR_SUCCESS));

        client = std::make_unique<EdgeVoxControlClient>();
        received_messages.clear();
    }

    void TearDown() override {
        if (client && client->is_connected()) {
            client->disconnect();
        }
        client.reset();
        ::testing::Mock::VerifyAndClearExpectations(mock_mosquitto);
        stored_connect_callback = nullptr;
        stored_message_callback = nullptr;
        stored_userdata = nullptr;
    }

    std::unique_ptr<EdgeVoxControlClient> client;
    MockMosquitto* mock_mosquitto;
    std::vector<std::string> received_messages;

    void set_test_callback() {
        client->set_status_callback(
            [this](const std::string& status) { received_messages.push_back(status); });
    }
};

TEST_F(EdgeVoxControlClientTest, InitializationTest) {
    EXPECT_FALSE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, SuccessfulConnectionTest) {
    using ::testing::_;
    using ::testing::Return;

    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    set_test_callback();
    EXPECT_TRUE(client->connect("test.mosquitto.org", 1883));

    // Trigger the connect callback with success
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    EXPECT_TRUE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, ConnectionFailureTest) {
    using ::testing::_;
    using ::testing::Return;

    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_CONN_REFUSED));

    EXPECT_FALSE(client->connect("test.mosquitto.org", 1883));
    EXPECT_FALSE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, DisconnectionTest) {
    using ::testing::_;
    using ::testing::Return;

    // Setup successful connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    ASSERT_TRUE(client->is_connected());

    client->disconnect();
    EXPECT_FALSE(client->is_connected());
}

TEST_F(EdgeVoxControlClientTest, SendCommandTest) {
    using ::testing::_;
    using ::testing::Return;

    // Setup successful connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, publish(_, _, _, _, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    ASSERT_TRUE(client->is_connected()) << "Client should be connected before sending command";
    EXPECT_TRUE(client->send_command("test_command"));
}

TEST_F(EdgeVoxControlClientTest, SendCommandWhileDisconnectedTest) {
    EXPECT_FALSE(client->send_command("test_command"));
}

TEST_F(EdgeVoxControlClientTest, PublishFailureTest) {
    using ::testing::_;
    using ::testing::Return;

    // Setup successful connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, publish(_, _, _, _, _, _)).WillOnce(Return(MOSQ_ERR_PROTOCOL));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    ASSERT_TRUE(client->is_connected());
    EXPECT_FALSE(client->send_command("test_command"));
}

TEST_F(EdgeVoxControlClientTest, ConnectionLostTest) {
    using ::testing::_;
    using ::testing::Return;

    // Setup successful connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    // Expect a publish attempt that should fail
    EXPECT_CALL(*mock_mosquitto, publish(_, _, _, _, _, _)).WillOnce(Return(MOSQ_ERR_NO_CONN));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    ASSERT_TRUE(client->is_connected());

    // Simulate connection lost by triggering on_disconnect callback
    if (stored_connect_callback) {
        stored_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                stored_userdata, MOSQ_ERR_CONN_LOST);
    }

    // Try to send command after connection is lost - should fail
    EXPECT_FALSE(client->send_command("test_command"));
}

TEST_F(EdgeVoxControlClientTest, MessageCallbackTest) {
    using ::testing::_;
    using ::testing::Return;

    std::string received_status;
    client->set_status_callback(
        [&received_status](const std::string& status) { received_status = status; });

    // Setup successful connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);

    // Create a test message
    struct mosquitto_message test_msg = {};
    const char* payload = "Test Message";
    test_msg.payload = const_cast<void*>(static_cast<const void*>(payload));
    test_msg.payloadlen = strlen(payload);

    // Trigger message callback
    mock_mosquitto->trigger_message_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             &test_msg);
    EXPECT_EQ(received_status, "Test Message");
}

TEST_F(EdgeVoxControlClientTest, ReconnectionTest) {
    using ::testing::_;
    using ::testing::Return;

    // First connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    ASSERT_TRUE(client->is_connected());

    client->disconnect();
    ASSERT_FALSE(client->is_connected());

    // Second connection
    EXPECT_CALL(*mock_mosquitto, connect(_, _, _)).WillOnce(Return(MOSQ_ERR_SUCCESS));
    EXPECT_CALL(*mock_mosquitto, loop_start()).WillOnce(Return(MOSQ_ERR_SUCCESS));

    ASSERT_TRUE(client->connect("test.mosquitto.org", 1883));
    mock_mosquitto->trigger_connect_callback(reinterpret_cast<struct mosquitto*>(mock_mosquitto),
                                             0);
    EXPECT_TRUE(client->is_connected());
}
