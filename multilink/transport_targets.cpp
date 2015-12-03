#include "multilink/transport_targets.h"
#include "libreactor/packet_stream_util.h"
#define LOG_NAME "transport_targets"
#include "libreactor/logging.h"
#include "libreactor/misc.h"

TargetCreator create_connecting_tcp_target_creator(Reactor& reactor, std::string host, int port) {
    return [host, port, &reactor](uint64_t id) -> Future<std::shared_ptr<PacketStream> > {
        return TCP::connect(reactor, host, port).then<std::shared_ptr<PacketStream> >([&reactor](FDPtr fd) -> std::shared_ptr<PacketStream> {
            set_recv_buffer(fd, 1024 * 32); // FIXME: possibly remove this
            return FreePacketStream::create(reactor, fd);
        });
    };
}

void create_listening_tcp_target_creator(Reactor& reactor, std::shared_ptr<Transport> transport, std::string host, int port) {
    std::shared_ptr<uint64_t> next_id = std::make_shared<uint64_t>(0);

    TCP::listen(reactor, host, port, [&reactor, next_id, transport] (FDPtr fd) {
        uint64_t& id_ref = *(next_id);
        uint64_t id = id_ref++;
        LOG("incoming connection, assigned id: " << id);

        auto stream = FreePacketStream::create(reactor, fd);
        transport->add_target(id, Future<std::shared_ptr<PacketStream> >::make_immediate(stream));
    });
}

TargetCreator unknown_stream_target_creator() {
    return [](uint64_t) -> Future<std::shared_ptr<PacketStream> > {
        // TODO: report error
        LOG("use of unknown stream");
        return make_future(std::shared_ptr<PacketStream> {});
    };
}
