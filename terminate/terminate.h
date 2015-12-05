#ifndef TERMINATE_H_
#define TERMINATE_H_
#include "multilink/transport.h"
#include "terminate/tun.h"
#include "terminate/lwip_tcp.h"

class Terminator : public std::enable_shared_from_this<Terminator> {
    bool is_server;
    Reactor& reactor;
    NetworkInterfacePtr network_interface;
    TcpListenerPtr tcp_listener;
    uint64_t id_counter;

    Terminator(Reactor& reactor);

    std::weak_ptr<Transport> transport;
    std::weak_ptr<Tun> tun;

    void tun_recv_ready();
    Future<PacketStreamPtr> create_target_2(Buffer data);
    void tcp_accepted(TcpStreamPtr stream);
public:
    static std::shared_ptr<Terminator> create(Reactor& reactor, bool is_server);

    Future<std::shared_ptr<PacketStream> > create_target(uint64_t id);
    void set_transport(TransportPtr transport);
    void set_tun(TunPtr tun);
};

typedef std::shared_ptr<Terminator> TerminatorPtr;

#endif
