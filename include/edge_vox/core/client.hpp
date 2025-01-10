#pragma once

namespace edge_vox {

class Client {
public:
    Client();
    ~Client();

    bool connect(const char* host, int port);
};

}  // namespace edge_vox