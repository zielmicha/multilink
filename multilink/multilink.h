#ifndef MULTILINK_H
#define MULTILINK_H
#include "multilink_link.h"
#include "packet_queue.h"

namespace Multilink {
    class Multilink: public AbstractPacketStream {
        // Reliability:
        // - all packets eventually arrive
        // - packets may arrive reordered
        // - packets may arrive duplicated
    private:
        Reactor& reactor;

        std::vector<std::unique_ptr<Link> > links;

        PacketQueue queue;
        uint64_t last_seq = 0; // TODO

        void link_recv_ready(Link* link);
        void link_send_ready(Link* link);
        void some_link_send_ready();

        void shuffle_links();

    public:
        Multilink(Reactor& reactor);
        Multilink(const Multilink& link) = delete;

        Link& add_link(Stream* stream, std::string name = "default");

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
