#include "rpc.h"
#include "unix_socket.h"
#include "ioutil.h"
#include "json11.hpp"
#include <iostream>

using json11::Json;

RPCServer::RPCServer(Reactor& reactor, OnMessageCallback on_message):
    reactor(reactor), on_message(on_message) {

}

std::shared_ptr<RPCServer> RPCServer::create(
    Reactor& reactor, std::string path, OnMessageCallback on_message) {
    std::shared_ptr<RPCServer> srv {new RPCServer(reactor, on_message)};
    UnixSocket::listen(reactor, path, [srv](FD* fd) {
        srv->accept(fd);
    });
    return srv;
}

void RPCServer::accept(FD* fd) {
    ioutil::read(fd, 4).then<Buffer>([=](Buffer data) {
        uint32_t length = data.convert<uint32_t>(0);
        return ioutil::read(fd, length);
    }).then<unit>([=](Buffer data) {
        std::string error;
        auto msg = Json::parse(data, error);
        std::cerr << "packet " << msg.dump() << std::endl;
        return unit();
    });
}
