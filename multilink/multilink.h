#ifndef MULTILINK_H
#define MULTILINK_H
#include <cstdint>
#include <string>
#include <iostream>
#include <boost/optional/optional.hpp>
#include "reactor.h"
#include "timer.h"
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

        AllocBuffer recv_buffer_alloc;
        Buffer recv_buffer;
        size_t recv_buffer_pos = 0;

        AllocBuffer waiting_recv_packet_buffer;
        optional<Buffer> waiting_recv_packet;

        AllocBuffer send_buffer;
        Buffer send_buffer_current;

        uint64_t last_ping_sent = 0;
        uint64_t last_pong_request_time = 0;
        uint64_t last_pong_request_seq = 0;
        uint64_t last_seq_sent = 0;

        void transport_write_ready();
        void transport_read_ready();

        bool try_parse_recv_packet();
        void parse_recv_packet(Buffer data);

        bool should_ping();
        void send_ping();
        void send_pong();
        bool send_aux();

        void format_send_packet(uint8_t type, Buffer data);

    public:
        Link(Reactor& reactor, Stream* stream);
        ~Link();
        Link(const Link& l) = delete;

        std::string name;

        Stats rtt;
        BandwidthEstimator bandwidth;

        void display(std::ostream& stream) const;

        optional<Buffer> recv(); // return value is valid only until next cycle
        bool send(uint64_t seq, const Buffer data);

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
