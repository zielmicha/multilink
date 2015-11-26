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

    FD* target = TCP::connect(reactor, "127.0.0.1", 6001).wait(reactor);
    LOG("connected");

    Multilink::Multilink mlink {reactor};

    int link_counter = 0;

    TCP::listen(reactor, "0.0.0.0", 6000, [&](FD* sock) {
        LOG("link connected");
        Stream* link = new ThrottledStream(reactor, sock, 5);
        mlink.add_link(link, boost::lexical_cast<std::string>(link_counter ++));
    }).ignore();

    ReorderStream out {&mlink};
    auto ptarget = FreePacketStream::create(reactor, target);

    pipe(reactor, &*ptarget, &out);
    pipe(reactor, &out, &*ptarget);

    reactor.run();
}
