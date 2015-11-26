#include "libreactor/reactor.h"
#include "libreactor/tcp.h"
#include <boost/lexical_cast.hpp>
#include "libreactor/logging.h"
#include "libreactor/misc.h"
#include "multilink/reorder_stream.h"
#include "multilink/multilink.h"
#include "libreactor/throttled.h"
#include "libreactor/packet_stream_util.h"

int main() {
    Reactor reactor;

    Completer<FD*> client_future;

    TCP::listen(reactor, "0.0.0.0", 6002, [&](FD* sock) {
        LOG("client connected");
        client_future.result(sock);
    }).ignore();

    FD* client = client_future.future().wait(reactor);

    Multilink::Multilink mlink {reactor};

    TCP::connect(reactor, "127.0.0.1", 6000).on_success([&](FD* sock) {
        LOG("link connected");
        Stream* link = new ThrottledStream(reactor, sock, 2);
        mlink.add_link(link, "LINK!");
    });

    ReorderStream out {&mlink};
    auto pclient = FreePacketStream::create(reactor, client);

    pipe(reactor, &*pclient, &out);
    pipe(reactor, &out, &*pclient);

    reactor.run();
}
