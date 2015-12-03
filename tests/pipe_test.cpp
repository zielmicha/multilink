#include "libreactor/reactor.h"
#include "libreactor/packet_stream_util.h"
#include "libreactor/misc.h"
#include "libreactor/exfunctional.h"
#include "libreactor/logging.h"
#include "libreactor/timer.h"

int main() {
    Reactor reactor;
    std::vector<FDPtr> fds1 = fd_pair(reactor);
    std::vector<FDPtr> fds2 = fd_pair(reactor);
    std::function<std::shared_ptr<FreePacketStream>(FDPtr)> make_packet_stream = [&](FDPtr fd) {
        return FreePacketStream::create(reactor, fd);
    };
    auto p1 = mapvector(fds1, make_packet_stream);
    auto p2 = mapvector(fds2, make_packet_stream);

    pipe(reactor, &*(p1[0]), &*(p2[0]));
    pipe(reactor, &*(p2[0]), &*(p1[0]));

    auto install_recv_debug = [](PacketStream* stream) {
        stream->set_on_recv_ready([stream]() {
            while(true) {
                auto d = stream->recv();
                if(!d) break;
                LOG("recv " << *d);
            }
        });
        stream->set_on_send_ready([](){});
    };

    install_recv_debug(&*(p1[1]));
    install_recv_debug(&*(p2[1]));


    assert(p1[1]->send(Buffer::from_cstr("foobar")));
    assert(!p1[1]->send(Buffer::from_cstr("foobar")));

    Timer timer(reactor);

    timer.once(1000 * 1000, [&]() {
        assert(p2[1]->send(Buffer::from_cstr("foobar")));
    });

    reactor.run();
}
