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
        uint64_t last_sent_packet = 0;
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

    std::shared_ptr<ChildStream> get_child_stream(uint64_t id);
    void create_child_stream(uint64_t id);

    void target_send_ready(std::shared_ptr<ChildStream> child);
    void target_recv_ready(std::shared_ptr<ChildStream> child);
    bool target_recv_ready_once(std::shared_ptr<ChildStream> child);

    void place_packet(uint64_t id, uint64_t seq, Buffer data);

    void network_send_ready();
    void network_recv_ready();

public:
    static std::shared_ptr<Transport> create(
        Reactor& reactor,
        std::shared_ptr<PacketStream> network_stream,
        TargetCreator target_creator, size_t mtu);

    void add_target(uint64_t id, Future<std::shared_ptr<PacketStream> > stream);
};

#endif
