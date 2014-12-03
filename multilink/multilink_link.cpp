#include "multilink.h"
#define LOGGER_NAME "link"
#include "logging.h"
#include <arpa/inet.h>

const int64_t SAMPLING_TIME = 2000;
const size_t MTU = 4096;
const size_t HEADER_SIZE = 3;

namespace Multilink {
    Link::Link(Reactor& reactor, Stream* stream): reactor(reactor),
                                                  stream(stream),
                                                  recv_buffer_alloc(MTU),
                                                  recv_buffer(recv_buffer_alloc.as_buffer()),
                                                  waiting_recv_packet_buffer(MTU),
                                                  send_buffer(MTU),
                                                  send_buffer_current(NULL, 0),
                                                  bandwidth(SAMPLING_TIME) {
        stream->set_on_read_ready(std::bind(&Link::transport_read_ready, this));
        stream->set_on_write_ready(std::bind(&Link::transport_write_ready, this));
    }

    void Link::transport_write_ready() {
        //LOG("transport_write_ready: pre");
        while(true) {
            if(send_buffer_current.size == 0) {
                reactor.schedule([this]() {
                    this->on_send_ready();
                });
                return;
            }

            //LOG("transport_write_ready");
            while(send_buffer_current.size != 0) {
                size_t bytes = stream->write(send_buffer_current);
                //LOG("just sent " << send_buffer_current.slice(0, bytes));
                if(bytes == 0) return;
                send_buffer_current = send_buffer_current.slice(bytes);
            }
        }
    }

    bool Link::send(Buffer data) {
        if(send_buffer_current.size != 0)
            return false; // previous packet still not sent

        format_send_packet(0, data);
        transport_write_ready();

        return true;
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
            if(waiting_recv_packet) // out buffer is occupied
                break;

            Buffer new_buffer = stream->read(recv_buffer.slice(recv_buffer_pos));
            if(new_buffer.size == 0) // nothing read
                break;

            recv_buffer_pos += new_buffer.size;

            //LOG("just read " << new_buffer);

            while(try_parse_recv_packet()) ;
        }
    }

    bool Link::try_parse_recv_packet() {
        if(recv_buffer_pos >= sizeof(uint16_t)) {
            uint16_t expected_length = ntohs(recv_buffer.convert<uint16_t>(0));
            if(expected_length > recv_buffer.size) {
                LOG("expected_length too big");
            }
            if(recv_buffer_pos >= expected_length) {
                // packet is ready
                parse_recv_packet(recv_buffer.slice(0, expected_length));
                recv_buffer.slice(0, recv_buffer_pos).delete_start(expected_length);
                recv_buffer_pos -= expected_length;
            }
            return true;
        }
        return false;
    }

    void Link::parse_recv_packet(Buffer data) {
        uint8_t type = data.convert<uint8_t>(2);
        if(type == 0) {
            waiting_recv_packet = waiting_recv_packet_buffer.as_buffer().slice(0, data.size - HEADER_SIZE);
            data.slice(HEADER_SIZE).copy_to(*waiting_recv_packet);
            on_recv_ready();
        }
    }

    optional<Buffer> Link::recv() {
        optional<Buffer> packet;
        std::swap(packet, waiting_recv_packet);
        // recv buffer is now empty - attempt reading from socket
        // Schedule transport_read_ready, so it won't overwrite waiting_recv_packet.
        // (After congestion on recv/on_recv_ready side there may be waiting data in stream)
        reactor.schedule(std::bind(&Link::transport_read_ready, this));
        return packet;
    }

    Link::~Link() {
        stream->close();
    }
}
