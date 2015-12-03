#ifndef RPC_H_
#define RPC_H_
#include "libreactor/reactor.h"
#include <memory>
#include "json11.hpp"
#include "libreactor/future.h"

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

    void accept(FDPtr sock);
    void read_message(std::shared_ptr<RPCStream> stream);
public:
    static std::shared_ptr<RPCServer> create(
        Reactor& reactor,
        std::string path,
        OnMessageCallback on_message);
    static std::shared_ptr<RPCServer> create(
        Reactor& reactor,
        int fd,
        OnMessageCallback on_message);
};

class RPCStream {
    FDPtr fd;
    Reactor& reactor;
    bool abandoned = false;

    RPCStream(Reactor& reactor):
        reactor(reactor) {}

    friend class RPCServer;
public:
    FDPtr abandon() {
        abandoned = true;
        return fd;
    }

    Future<int> recv_fd();
    void write(Json data);
};

#endif
