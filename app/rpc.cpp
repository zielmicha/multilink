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
    std::cerr << "accepted unix connection fd=" << fd->fileno() << std::endl;
    ioutil::read(fd, 4).then<ByteString>([=](ByteString data) {
        uint32_t length = data.as_buffer().convert<uint32_t>(0);
        std::cerr << "read " << length << std::endl;
        return ioutil::read(fd, length);
    }).then<unit>([=](ByteString data) {
        std::string error;
        auto msg = Json::parse(std::string(data), error);
        std::cerr << "packet " << msg.dump() << std::endl;
        return unit();
    });
}
