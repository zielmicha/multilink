#ifndef MULTILINK_H
#define MULTILINK_H
#include <cstdint>
#include <string>
#include <iostream>
#include <boost/optional/optional.hpp>
#include "reactor.h"
#include "multilink_stats.h"

namespace Multilink {
    template <class T>
    using optional = boost::optional<T>;

    struct ChannelInfo {
        uint8_t type;
        uint64_t id;

        enum {
            TYPE_COMMON = 0x01,
            TYPE_TCP = 0x02,
            TYPE_RAW = 0x81,
        };
    };

    class Link {
    private:
        Reactor& reactor;
        Stream* stream;

        uint64_t last_recv_id;
        uint64_t last_recv_time;
        uint64_t last_recv_ack_id;

        AllocBuffer recv_buffer;
        Buffer recv_buffer_current;

        AllocBuffer send_buffer;
        Buffer send_buffer_current;

        void transport_write_ready();
        void transport_read_ready();

        void try_parse_recv_packet();
        void parse_recv_packet(Buffer data);

        void format_send_packet(Buffer data);

    public:
        Link(Reactor& reactor, Stream* stream);
        ~Link();
        Link(const Link& l) = delete;

        std::string name;

        Stats rtt;
        BandwidthEstimator bandwidth;

        void display(std::ostream& stream) const;

        optional<Buffer> recv(Buffer data);
        bool send(const Buffer data);

        std::function<void()> on_recv_ready;
        std::function<void()> on_send_ready;

        friend std::ostream& operator<<(std::ostream& stream, const Link& link) {
            link.display(stream);
            return stream;
        }
    };

    class Multilink {
    private:
        std::vector<Link> links;
    public:
        Multilink();
        Multilink(const Multilink& link) = delete;

        Link& add_link(Stream* stream, std::string name = "default");

        std::function<void()> on_send_ready;
        std::function<void()> on_recv_ready;

        /** Queue packet to be sent. Always succeeds */
        void send(ChannelInfo channel, const char* buff, size_t length);
        /** Receive packet, if there is one waiting.
         * Returns packet size or -1 if no packet is ready to be received.
         */
        ssize_t recv(ChannelInfo& channel, const char* buff, size_t length);
    };
}
#endif
