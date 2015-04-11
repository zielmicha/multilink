#ifndef RPC_H_
#define RPC_H_
#include "reactor.h"
#include <memory>
#include "json11.hpp"

using namespace json11;

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
public:
    static std::shared_ptr<RPCServer> create(
        Reactor& reactor,
        std::string path,
        OnMessageCallback on_message);

};

class RPCStream {
public:
    FD* abandon();
    void write(Json data);
};

#endif
