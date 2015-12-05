#ifndef TERMINATE_TUN_H_
#define TERMINATE_TUN_H_

#include <string>

#include "libreactor/common.h"
#include "libreactor/reactor.h"
#include "libreactor/packet_stream.h"

class Tun : public AbstractPacketStream, public std::enable_shared_from_this<Tun> {
protected:
    Reactor& reactor;
    FDPtr fd;
    AllocBuffer buffer;

    void transport_ready_ready();
public:
    Tun(Reactor& r);
    Tun(const Tun& t) = delete;

    static std::shared_ptr<Tun> create(Reactor& r, std::string name);

    bool is_send_ready();
    void send_with_offset(Buffer data) { send(data); }
    size_t get_send_offset() { return 0; }
    void send(Buffer data);
    optional<Buffer> recv();
    void close();
    std::string name;
};

typedef std::shared_ptr<Tun> TunPtr;

#endif
