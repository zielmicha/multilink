#ifndef MULTILINK_H
#define MULTILINK_H
#include "multilink/multilink_link.h"
#include "multilink/packet_queue.h"

namespace Multilink {
    class Multilink: public AbstractPacketStream {
        // Reliability:
        // - all packets eventually arrive
        // - packets may arrive reordered
        // - packets may arrive duplicated
    private:
        Reactor& reactor;

        std::vector<std::unique_ptr<Link> > links;
        std::unordered_map<Link*, std::function<void()> > link_on_close;

        std::deque<AllocBuffer> queue;
        std::unordered_map<Link*, std::deque<AllocBuffer> > assigned_packets;

        uint64_t last_seq = 0; // TODO

        void link_recv_ready(Link* link);
        void link_send_ready(Link* link);
        void some_link_send_ready();

        void shuffle_links();

        void assign_links_until(Link* link);
        void request_packets(Link* link);
    public:
        Multilink(Reactor& reactor);
        Multilink(const Multilink& link) = delete;

        Link& add_link(StreamPtr stream, std::string name = "default",
                       std::function<void()> on_close = nothing);

        bool is_send_ready();

        /** Queue packet to be sent. */
        void send_with_offset(const Buffer data);

        size_t get_send_offset() { return 0; }

        /** Receive packet, if there is one waiting.
         * Returns temp pointer or none if no packet is waiting to be received.
         */
        optional<Buffer> recv();

        void close() {}
    };
}
#endif
