#ifndef MULTILINK_LINK_H
#define MULTILINK_LINK_H
#include <cstdint>
#include <string>
#include <iostream>
#include "reactor.h"
#include "timer.h"
#include "multilink_stats.h"
#include "packet_stream.h"

namespace Multilink {
    const size_t LINK_MTU = 4096;
    const size_t HEADER_SIZE = 3;
    const size_t MULTILINK_MTU = LINK_MTU - HEADER_SIZE;

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
        Timer timer;

        AllocBuffer recv_buffer_alloc;
        Buffer recv_buffer;
        size_t recv_buffer_pos = 0;

        AllocBuffer waiting_recv_packet_buffer;
        optional<Buffer> waiting_recv_packet;

        AllocBuffer send_buffer;
        Buffer send_buffer_current;
        bool send_buffer_edge = true;

        uint64_t last_ping_sent = 0;
        uint64_t last_pong_request_time = 0;
        uint64_t last_pong_request_seq = 0;
        uint64_t last_seq_sent = 0;
        uint64_t last_pong_recv_seq = 0;

        void timer_callback();
        void transport_write_ready(bool real=false);
        void transport_read_ready();

        bool try_parse_recv_packet();
        void parse_recv_packet(Buffer data);

        bool should_ping();
        void send_ping();
        void send_pong();
        bool send_aux();

        void format_send_packet(uint8_t type, Buffer data);
        void raw_send_packet(uint8_t type, Buffer data);

        function<void()> on_send_ready_edge_fn;
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
        uint64_t get_last_ack_seq() { return last_pong_recv_seq; };

        function<void()> on_recv_ready;
        function<void()> on_send_ready;

        friend std::ostream& operator<<(std::ostream& stream, const Link& link) {
            link.display(stream);
            return stream;
        }
    };
}

#endif
