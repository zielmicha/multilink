#ifndef TRANSPORT_H_
#define TRANSPORT_H_
#include "packet_stream.h"
#include "future.h"
#include <memory>
#include <unordered_map>
#include <deque>
/** Embedded transport layer
 *
 * Handles multiplexing, reordering and flow control.
 * Multipurpose for simplicity and efficiency.
 */

typedef std::function<Future<std::shared_ptr<PacketStream> >(uint64_t)> TargetCreator;

class Transport : public std::enable_shared_from_this<Transport> {
    class ChildStream {
        std::weak_ptr<Transport> transport;
        uint64_t id;
        std::shared_ptr<PacketStream> target;

        // reorder/forward side (sends to target)
        uint64_t used_buffer_size = 0;
        uint64_t last_forwarded_packet = 0;
        void check_forward();

        // send side (recvs from target)
        uint64_t last_sent_packet;
        bool choked;

        ChildStream();
        friend class Transport;

        std::deque<AllocBuffer> buffer;
    };

    std::shared_ptr<PacketStream> network_stream;
    std::unordered_map<uint64_t, std::shared_ptr<ChildStream> > children;
    TargetCreator target_creator;
    size_t mtu;

    Transport(std::shared_ptr<PacketStream> network_stream, TargetCreator target_creator, size_t mtu)
        : network_stream(network_stream), target_creator(target_creator), mtu(mtu) {}

    void create_child_stream(uint64_t id);
    void target_send_ready(std::shared_ptr<ChildStream> child);
    void target_recv_ready(std::shared_ptr<ChildStream> child);

    void network_send_ready();
    void network_recv_ready();

public:
    std::shared_ptr<Transport> create(std::shared_ptr<PacketStream> network_stream,
                                      TargetCreator target_creator, size_t mtu);
};

#endif
