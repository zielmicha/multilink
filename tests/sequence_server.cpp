#include "libreactor/tcp.h"
#include <boost/lexical_cast.hpp>
#include "libreactor/logging.h"
#include "libreactor/misc.h"
#include "multilink/reorder_stream.h"
#include "multilink/multilink.h"
#include "libreactor/throttled.h"

int main() {
    Reactor reactor;
    Multilink::Multilink mlink {reactor};

    int link_counter = 0;

    TCP::listen(reactor, "0.0.0.0", 6000, [&](FD* sock) {
        LOG("link connected");
        Stream* link = new ThrottledStream(reactor, sock, 2);
        mlink.add_link(link, boost::lexical_cast<std::string>(link_counter ++));
    }).ignore();

    int send_counter = 0;
    ReorderStream out {&mlink};

    out.on_send_ready = [&]() {
        //LOG("on send ready " << send_counter);
        while(true) {
            StackBuffer<400> sbuff;
            Buffer(sbuff).convert<int32_t>(0) = send_counter;
            if(out.send(sbuff))
                send_counter ++;
            else
                break;
        }
    };
    out.on_recv_ready = nothing;

    reactor.run();
}
