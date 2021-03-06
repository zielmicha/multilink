#include "app/rpc.h"
#include "libreactor/unix_socket.h"
#include "libreactor/ioutil.h"
#include "json11.hpp"
#include <iostream>

RPCServer::RPCServer(Reactor& reactor, OnMessageCallback on_message):
    reactor(reactor), on_message(on_message) {

}
std::shared_ptr<RPCServer> RPCServer::create(
    Reactor& reactor, std::string path, OnMessageCallback on_message) {
    std::shared_ptr<RPCServer> srv {new RPCServer(reactor, on_message)};
    UnixSocket::listen(reactor, path, [srv](FDPtr fd) {
        srv->accept(fd);
    });
    return srv;
}

std::shared_ptr<RPCServer> RPCServer::create(
    Reactor& reactor, int fd, OnMessageCallback on_message) {
    std::shared_ptr<RPCServer> srv {new RPCServer(reactor, on_message)};
    UnixSocket::listen(reactor, reactor.take_fd(fd), [srv](FDPtr fd) {
        srv->accept(fd);
    });
    return srv;
}

void RPCServer::accept(FDPtr fd) {
    std::shared_ptr<RPCStream> stream {new RPCStream(reactor)};
    std::cerr << "accepted unix connection fd=" << fd->fileno() << std::endl;
    stream->fd = fd;
    read_message(stream);
}

void RPCServer::read_message(std::shared_ptr<RPCStream> stream) {
    ioutil::read(stream->fd, 4).then<ByteString>([=](ByteString data) {
        uint32_t length = data.as_buffer().convert<uint32_t>(0);
        if (length > 1024 * 1024)
            return Future<ByteString>::make_exception("requested buffer too big");
        return ioutil::read(stream->fd, length);
    }).then<unit>([=](ByteString data) {
        std::string error;
        Json msg = Json::parse(std::string(data), error);

        on_message(stream, msg);

        if(!stream->abandoned)
            read_message(stream);
        return unit();
    });
}

void RPCStream::write(Json data) {
    assert(!abandoned);
    assert(false); // TODO
}

Future<int> RPCStream::recv_fd() {
    abandoned = true;
    return UnixSocket::recv_fd(reactor, fd).then<int>([=](int fd) -> int {
        return fd;
    });
}
