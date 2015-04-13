#ifndef RPC_H_
#define RPC_H_
#include "reactor.h"
#include <memory>
#include "json11.hpp"

using json11::Json;

class RPCStream;

class RPCServer {
public:
    typedef
    std::function<void(std::shared_ptr<RPCStream>, Json message)>
    OnMessageCallback;

private:
    Reactor& reactor;
    OnMessageCallback on_message;

    RPCServer(Reactor& reactor,
              OnMessageCallback on_message);

    void accept(FD* sock);
    void read_message(std::shared_ptr<RPCStream> stream);
public:
    static std::shared_ptr<RPCServer> create(
        Reactor& reactor,
        std::string path,
        OnMessageCallback on_message);

};

class RPCStream {
    FD* fd;
    bool abandoned = false;

    friend class RPCServer;
public:
    FD* abandon() {
        abandoned = true;
        return fd;
    }

    void write(Json data);
};

#endif
