#include "rpc.h"
#include "unix_socket.h"
#include "ioutil.h"
#include "json11.hpp"
#include <iostream>

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
    std::shared_ptr<RPCStream> stream = std::make_shared<RPCStream>();
    std::cerr << "accepted unix connection fd=" << fd->fileno() << std::endl;
    stream->fd = fd;
    read_message(stream);
}

void RPCServer::read_message(std::shared_ptr<RPCStream> stream) {
    ioutil::read(stream->fd, 4).then<ByteString>([=](ByteString data) {
        uint32_t length = data.as_buffer().convert<uint32_t>(0);
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
