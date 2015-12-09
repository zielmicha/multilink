#ifndef _TERMINATE_IPPACKET
#define _TERMINATE_IPPACKET
#include "terminate/tun.h"
#include "terminate/address.h"
#include <deque>

class RawPacketStream : public std::enable_shared_from_this<RawPacketStream>,
                        public AbstractPacketStream {

    bool is_server;
    std::weak_ptr<Tun> tun;
    std::deque<AllocBuffer> from_tun_queue;
    AllocBuffer from_tun_current;
    AllocBuffer output_buffer;

    bool check_udp(Buffer payload);
    bool prepare_tun_packet(Buffer packet);

    IpAddress ip4;

public:
    RawPacketStream(bool is_server);

    void set_tun(TunPtr tun, std::string ip);
    bool from_tun(Buffer data);

    bool is_send_ready();
    void send(const Buffer data);
    optional<Buffer> recv();

    size_t get_send_offset() { return 0; }
    void send_with_offset(const Buffer data) { send(data); }
    void close() {}
};

typedef std::shared_ptr<RawPacketStream> RawPacketStreamPtr;

#endif
