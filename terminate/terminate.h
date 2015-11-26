#ifndef TERMINATE_H_
#define TERMINATE_H_
#include "multilink/transport.h"
#include "terminate/tun.h"

class Terminator : std::enable_shared_from_this<Terminator> {
    Terminator(Reactor& reactor);
public:
    std::shared_ptr<Terminator> create(Reactor& reactor);

    Future<std::shared_ptr<PacketStream> > create_target(uint64_t id);
    void set_transport(TransportPtr transport);
    void set_tun(TunPtr tun);
};

typedef std::shared_ptr<Terminator> TerminatorPtr;

#endif
