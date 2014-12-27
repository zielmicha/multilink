#ifndef MULTILINK_H
#define MULTILINK_H
#include "multilink_link.h"
#include "packet_queue.h"

namespace Multilink {
    class Multilink {
        // Reliability:
        // - all packets eventually arrive
        // - packets may arrive reordered
        // - packets may arrive duplicated
    private:
        Reactor& reactor;

        std::vector<std::unique_ptr<Link> > links;
        uint64_t buffsize = 10000;

        PacketQueue queue;

        void link_recv_ready(Link* link);
        void link_send_ready(Link* link);

    public:
        Multilink(Reactor& reactor);
        Multilink(const Multilink& link) = delete;

        Link& add_link(Stream* stream, std::string name = "default");

        std::function<void()> on_send_ready;
        std::function<void()> on_recv_ready;

        /** Queue packet to be sent. Returns true if suceeded. */
        bool send(const Buffer data);

        /** Receive packet, if there is one waiting.
         * Returns temp pointer or none if no packet is waiting to be received.
         */
        optional<Buffer> recv();
    };
}
#endif
