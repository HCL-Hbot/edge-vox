#pragma once
#include <functional>
#include <memory>
#include <string>

class EdgeVoxControlClient {
public:
    EdgeVoxControlClient();
    ~EdgeVoxControlClient();

    bool connect(const std::string& host, uint16_t port);
    void disconnect();
    bool is_connected() const;
    void set_status_callback(std::function<void(const std::string&)> callback);
    bool send_command(const std::string& command);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};