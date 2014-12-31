#include "reactor.h"
#include "tcp.h"
#include <boost/lexical_cast.hpp>
#include "logging.h"
#include "misc.h"
#include "reorder_stream.h"
#include "multilink.h"
#include "throttled.h"
#include "packet_stream_util.h"

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

    pipe(target, &out);
    pipe(&out, target);

    reactor.run();
}
