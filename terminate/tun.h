#ifndef TERMINATE_TUN_H_
#define TERMINATE_TUN_H_

#include <string>

#include "libreactor/common.h"
#include "libreactor/reactor.h"
#include "libreactor/packet_stream.h"

class Tun : public AbstractPacketStream {
protected:
    Reactor& reactor;
    FDPtr fd;
    AllocBuffer buffer;

    void transport_ready_ready();
public:
    Tun(Reactor& r, std::string name);
    Tun(const Tun& t) = delete;

    bool is_send_ready();
    void send(Buffer buffer);
    optional<Buffer> recv();
    std::string name;
};

typedef std::shared_ptr<Tun> TunPtr;

#endif
