#include "tcp.h"
#include <boost/lexical_cast.hpp>
#include "logging.h"
#include "misc.h"
#include "reorder_stream.h"
#include "multilink.h"

int main() {
    Reactor reactor;
    Multilink::Multilink mlink {reactor};

    ReorderStream stream {&mlink};

    int32_t counter = 0;

    stream.on_send_ready = nothing;
    stream.on_recv_ready = [&] {
        while(true) {
            auto packet = stream.recv();
            if(!packet) break;
            auto v = packet->convert<int32_t>(0);
            assert(v == counter);
            counter ++;
        }
    };

    for(int i=0; i < 3; i++) {
        FD* conn = TCP::connect(reactor, "127.0.0.1", 6000).wait(reactor);
        mlink.add_link(conn, boost::lexical_cast<std::string>(i));
    }

    reactor.run();
}
