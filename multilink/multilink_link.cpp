#include "multilink.h"
#define LOGGER_NAME "link"
#include "logging.h"
#include <arpa/inet.h>

const size_t MTU = 4096;
const size_t HEADER_SIZE = 3;
const uint64_t PING_INTERVAL = 1000 * 1000;

namespace Multilink {
    Link::Link(Reactor& reactor, Stream* stream): reactor(reactor),
                                                  stream(stream),
                                                  timer(reactor),
                                                  recv_buffer_alloc(MTU),
                                                  recv_buffer(recv_buffer_alloc.as_buffer()),
                                                  waiting_recv_packet_buffer(MTU),
                                                  send_buffer(MTU),
                                                  send_buffer_current(NULL, 0) {
        stream->set_on_read_ready(std::bind(&Link::transport_read_ready, this));
        stream->set_on_write_ready(std::bind(&Link::transport_write_ready, this));
        reactor.schedule(std::bind(&Link::timer_callback, this));
    }

    void Link::display(std::ostream& out) const {
        out << "Link(" << name << ")";
    }

    void Link::timer_callback() {
        // Make sure pings are sent even when there is no other traffic.
        send_aux();
        timer.once(PING_INTERVAL, std::bind(&Link::timer_callback, this));
    }

    void Link::transport_write_ready() {
        bandwidth.write_ready(Timer::get_time());

        while(true) {
            if(send_buffer_current.size == 0) {
                // Send buffer is empty - schedule new packet.
                reactor.schedule([this]() {
                    this->send_aux();
                    this->on_send_ready();
                });
                return;
            }

            // Write as much as possible from send buffer.
            while(send_buffer_current.size != 0) {
                size_t bytes = stream->write(send_buffer_current);
                bandwidth.data_written(Timer::get_time(), bytes);
                if(bytes == 0) return;
                send_buffer_current = send_buffer_current.slice(bytes);
            }
        }
    }

    bool Link::send(uint64_t seq, Buffer data) {
        if(!send_aux()) {
            assert(seq > last_seq_sent);
            last_seq_sent = seq;
            format_send_packet(0, data);
            transport_write_ready();
            return true;
        } else {
            return false;
        }
    }

    bool Link::send_aux() {
        // Possibly send some auxiliary packet (ping/pong).
        // Returns false if after this call send buffer is empty.
        if(send_buffer_current.size != 0)
            return true; // previous packet still not sent

        if(last_pong_request_time != 0) {
            send_pong();
            transport_write_ready();
            return true;
        } else if(should_ping()) {
            send_ping();
            transport_write_ready();
            return true;
        } else {
            return false;
        }
    }

    bool Link::should_ping() {
        return (Timer::get_time() - last_ping_sent) > PING_INTERVAL;
    }

    void Link::send_ping() {
        LOG(*this << " send_ping");

        StackBuffer<16> data;
        Buffer(data).convert<uint64_t>(0) = Timer::get_time();
        Buffer(data).convert<uint64_t>(8) = last_seq_sent;
        format_send_packet(1, data);

        last_ping_sent = Timer::get_time();
    }

    void Link::send_pong() {
        StackBuffer<16> data;
        Buffer(data).convert<uint64_t>(0) = last_pong_request_time;
        Buffer(data).convert<uint64_t>(8) = last_pong_request_seq;
        format_send_packet(2, data);

        last_pong_request_time = 0;
        last_pong_request_seq = 0;
    }

    void Link::format_send_packet(uint8_t type, Buffer data) {
        send_buffer_current = send_buffer.as_buffer().slice(0, data.size + HEADER_SIZE);
        data.copy_to(send_buffer_current.slice(HEADER_SIZE));

        if(data.size > send_buffer.as_buffer().size)
            abort();

        send_buffer_current.convert<uint16_t>(0) = htons(data.size + HEADER_SIZE);
        send_buffer_current.convert<uint8_t>(2) = type;
    }

    void Link::transport_read_ready() {
        while(true) {
            if(waiting_recv_packet) // out buffer is occupied, wait until recv is called
                break;

            Buffer new_buffer = stream->read(recv_buffer.slice(recv_buffer_pos));
            if(new_buffer.size == 0) // nothing read
                break;

            recv_buffer_pos += new_buffer.size;

            // Parse zero or more packets.
            while(try_parse_recv_packet()) ;
        }
    }

    bool Link::try_parse_recv_packet() {
        if(recv_buffer_pos >= sizeof(uint16_t)) {
            uint16_t expected_length = ntohs(recv_buffer.convert<uint16_t>(0));
            if(expected_length > recv_buffer.size) {
                LOG("expected_length too big");
            }
            assert(expected_length != 0);

            if(recv_buffer_pos >= expected_length) {
                // packet is ready
                parse_recv_packet(recv_buffer.slice(0, expected_length));
                recv_buffer.slice(0, recv_buffer_pos).delete_start(expected_length);
                recv_buffer_pos -= expected_length;
                return true;
            }
        }
        return false;
    }

    void Link::parse_recv_packet(Buffer data) {
        uint8_t type = data.convert<uint8_t>(2);
        if(type == 0) { // data packet
            waiting_recv_packet = waiting_recv_packet_buffer.as_buffer().slice(0, data.size - HEADER_SIZE);
            data.slice(HEADER_SIZE).copy_to(*waiting_recv_packet);
            on_recv_ready();
        } else if(type == 1) { // ping
            last_pong_request_time = data.convert<uint64_t>(HEADER_SIZE);
            last_pong_request_seq = data.convert<uint64_t>(HEADER_SIZE + 8);
            reactor.schedule(std::bind(&Link::send_aux, this));
        } else if(type == 2) { // pong
            uint64_t time = data.convert<uint64_t>(HEADER_SIZE);
            uint64_t delta = Timer::get_time() - time;

            rtt.add_and_remove_back((int)delta);

            LOG("pong delta=" << delta << " rtt=" << rtt.mean() << " dev=" << rtt.stddev()
                << " bandwidth=" << (int)(bandwidth.bandwidth_mbps() * 8) << "Mbps");
            last_pong_recv_seq = data.convert<uint64_t>(HEADER_SIZE + 8);
        } else {
            LOG("unknown packet received: " << data);
        }
    }

    optional<Buffer> Link::recv() {
        optional<Buffer> packet;
        std::swap(packet, waiting_recv_packet);
        // recv buffer is now empty - attempt reading from socket
        // (After congestion on recv/on_recv_ready side there may be waiting data in stream)
        // (schedule the call, so it won't overwrite waiting_recv_packet)
        reactor.schedule(std::bind(&Link::transport_read_ready, this));
        return packet;
    }

    Link::~Link() {
        stream->close();
    }
}
