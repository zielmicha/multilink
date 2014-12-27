#ifndef MULTILINK_H
#define MULTILINK_H
#include "multilink_link.h"
#include "packet_queue.h"

namespace Multilink {
    class Multilink {
    private:
        Reactor& reactor;

        std::vector<std::unique_ptr<Link> > links;
        uint64_t buffsize = 10000;

        PacketQueue queue;
    public:
        Multilink(Reactor& reactor);
        Multilink(const Multilink& link) = delete;

        Link& add_link(Stream* stream, std::string name = "default");

        std::function<void()> on_send_ready;
        std::function<void()> on_recv_ready;

        /** Queue packet to be sent. Always succeeds */
        void send(ChannelInfo channel, const Buffer data);

        /** Receive packet, if there is one waiting.
         * Returns pointer to data or none if no packet is waiting to be received.
         */
        optional<Buffer> recv(ChannelInfo& channel, Buffer data);
    };
}
#endif
