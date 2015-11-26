#ifndef TERMINATE_H_
#define TERMINATE_H_
#include "multilink/transport.h"
#include "terminate/tun.h"

class Terminator : public std::enable_shared_from_this<Terminator> {
    bool is_server;
    Reactor& reactor;

    Terminator(Reactor& reactor);

    std::weak_ptr<Transport> transport;
    std::weak_ptr<Tun> tun;

    void tun_recv(Buffer packet);
    Future<PacketStreamPtr> create_target_2(Buffer data);
public:
    std::shared_ptr<Terminator> create(Reactor& reactor, bool is_server);

    Future<std::shared_ptr<PacketStream> > create_target(uint64_t id);
    void set_transport(TransportPtr transport);
    void set_tun(TunPtr tun);
};

typedef std::shared_ptr<Terminator> TerminatorPtr;

#endif
