#include "control_client.hpp"

#include <mosquitto.h>

#include <atomic>
#include <thread>

class EdgeVoxControlClient::Impl {
public:
    Impl() : mosq_(nullptr), connected_(false) {
        mosquitto_lib_init();
    }

    ~Impl() {
        disconnect();
        mosquitto_lib_cleanup();
    }

    static void on_connect(struct mosquitto* mosq, void* obj, int rc) {
        auto* self = static_cast<Impl*>(obj);
        if (rc == 0) {
            self->connected_ = true;
            if (self->status_callback_) {
                self->status_callback_("Connected to MQTT broker");
            }
        }
    }

    static void on_message(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {
        auto* self = static_cast<Impl*>(obj);
        if (self->status_callback_ && msg->payload) {
            std::string message(static_cast<char*>(msg->payload), msg->payloadlen);
            self->status_callback_(message);
        }
    }

    bool connect(const std::string& host, uint16_t port) {
        if (mosq_) {
            disconnect();
        }

        mosq_ = mosquitto_new(nullptr, true, this);
        if (!mosq_) {
            return false;
        }

        mosquitto_connect_callback_set(mosq_, on_connect);
        mosquitto_message_callback_set(mosq_, on_message);

        int rc = mosquitto_connect(mosq_, host.c_str(), port, 60);
        if (rc != MOSQ_ERR_SUCCESS) {
            mosquitto_destroy(mosq_);
            mosq_ = nullptr;
            return false;
        }

        mosquitto_loop_start(mosq_);
        return true;
    }

    void disconnect() {
        if (mosq_) {
            mosquitto_loop_stop(mosq_, true);
            mosquitto_disconnect(mosq_);
            mosquitto_destroy(mosq_);
            mosq_ = nullptr;
        }
        connected_ = false;
    }

    bool is_connected() const {
        return connected_;
    }

    void set_status_callback(std::function<void(const std::string&)> callback) {
        status_callback_ = std::move(callback);
    }

    bool send_command(const std::string& command) {
        if (!mosq_ || !connected_) {
            return false;
        }
        int rc = mosquitto_publish(mosq_, nullptr, "control", command.length(), command.c_str(), 0,
                                   false);
        return rc == MOSQ_ERR_SUCCESS;
    }

private:
    struct mosquitto* mosq_;
    std::atomic<bool> connected_;
    std::function<void(const std::string&)> status_callback_;
};

EdgeVoxControlClient::EdgeVoxControlClient() : pimpl_(std::make_unique<Impl>()) {}
EdgeVoxControlClient::~EdgeVoxControlClient() = default;

bool EdgeVoxControlClient::connect(const std::string& host, uint16_t port) {
    return pimpl_->connect(host, port);
}

void EdgeVoxControlClient::disconnect() {
    pimpl_->disconnect();
}

bool EdgeVoxControlClient::is_connected() const {
    return pimpl_->is_connected();
}

void EdgeVoxControlClient::set_status_callback(std::function<void(const std::string&)> callback) {
    pimpl_->set_status_callback(std::move(callback));
}

bool EdgeVoxControlClient::send_command(const std::string& command) {
    return pimpl_->send_command(command);
}