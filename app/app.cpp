#include "multilink.h"
#include <iostream>
#include "rpc.h"
#include "ioutil.h"

class Server {
    Reactor& reactor;
    std::unordered_map<int, FD*> unused_stream_fds;

public:
    Server(Reactor& reactor): reactor(reactor) {}

    void callback(std::shared_ptr<RPCStream> stream,
                  Json message) {
        std::string type = message["type"].string_value();
        std::cerr << "recv " << message.dump() << std::endl;

        if(type == "provide-stream") {
            int num = message["num"].int_value();
            FD* fd = stream->abandon();
            unused_stream_fds[num] = fd;
            ioutil::write(fd, ByteString::copy_from("ok\n"));
        } else {
            std::cerr << "bad type " << type << std::endl;
        }
    }
};

int main() {
    Reactor reactor;

    Server server {reactor};
    auto callback = [&](std::shared_ptr<RPCStream> stream,
                        Json message) {
        server.callback(stream, message);
    };
    auto rpcserver = RPCServer::create(reactor, "app.sock",
                                       callback);

    reactor.run();
}
