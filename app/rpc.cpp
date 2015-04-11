#include "rpc.h"
#include "unix_socket.h"

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

}
