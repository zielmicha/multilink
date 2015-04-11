#include "multilink.h"
#include <iostream>
#include "rpc.h"

int main() {
    Reactor reactor;

    auto callback = [&](std::shared_ptr<RPCStream> stream,
                        Json message) {
        std::cerr << "recv " << message.dump() << std::endl;
    };

    auto server = RPCServer::create(reactor, "app.sock", callback);

    reactor.run();
}
