#include "libreactor/tcp.h"
#include <boost/lexical_cast.hpp>
#include "libreactor/logging.h"
#include "libreactor/misc.h"
#include "multilink/reorder_stream.h"
#include "multilink/multilink.h"

int main(int argc, char** argv) {
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
            //LOG("recv " << v);
            assert(v == counter);
            counter ++;
        }
    };

    for(int i=0; i < 3; i++) {
        std::string host = "127.0.0.1";
        int port = 6000;
        if(argc == 3) {
            host = argv[1];
            port = atoi(argv[2]);
        }
        FD* conn = TCP::connect(reactor, host, port).wait(reactor);
        mlink.add_link(conn, boost::lexical_cast<std::string>(i));
    }

    reactor.run();
}
