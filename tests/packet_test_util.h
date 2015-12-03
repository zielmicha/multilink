#include "libreactor/packet_stream.h"
#include "libreactor/packet_stream_util.h"
#include "libreactor/exfunctional.h"
#include "libreactor/logging.h"
#include "libreactor/misc.h"

std::vector<std::shared_ptr<PacketStream> > packet_stream_pair(Reactor& reactor) {
    auto p = fd_pair(reactor);
    return mapvector(p, [&](FDPtr fd) -> std::shared_ptr<PacketStream> {
        return LengthPacketStream::create(reactor, fd);
    });
}

void packet_printer(Reactor& reactor, std::shared_ptr<PacketStream> stream, std::string name) {
    auto on_recv_ready = [name, stream]() {
        while(true) {
            auto packet = stream->recv();
            if(!packet) break;
            LOG(name << " recv " << *packet);
        }
    };
    stream->set_on_recv_ready(on_recv_ready);
    reactor.schedule(on_recv_ready);
}
