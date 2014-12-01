#include "multilink.h"
#define LOGGER_NAME "link"
#include "logging.h"
#include <arpa/inet.h>

const int64_t SAMPLING_TIME = 2000;
const size_t MTU = 4096;

namespace Multilink {
    Link::Link(Reactor& reactor, Stream* stream): reactor(reactor),
                                                  stream(stream),
                                                  recv_buffer(MTU),
                                                  recv_buffer_current(recv_buffer.as_buffer()),
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
                // TODO: may stack overflow happen here?
                // TODO: maybe use reactor.schedule
                // FIXME: Is this really needed? on_send_ready should call send in while loop
                on_send_ready();
                return;
            }

            //LOG("transport_write_ready");
            while(send_buffer_current.size != 0) {
                size_t bytes = stream->write(send_buffer_current);
                //LOG("just sent " << bytes);
                if(bytes == 0) return;
                send_buffer_current = send_buffer_current.slice(bytes);
            }
        }
    }

    bool Link::send(Buffer data) {
        if(send_buffer_current.size != 0)
            return false; // previous packet still not sent

        format_send_packet(data);
        transport_write_ready();

        return true;
    }

    void Link::format_send_packet(Buffer data) {
        const int header_size = 2;

        send_buffer_current = send_buffer.as_buffer().slice(0, data.size + header_size);
        data.copy_to(send_buffer_current.slice(header_size));

        if(data.size > send_buffer.as_buffer().size)
            abort();

        send_buffer_current.convert<uint16_t>(0) = htons(data.size + header_size);
    }

    void Link::transport_read_ready() {
        while(true) {
            Buffer new_buffer = stream->read(recv_buffer_current);
            if(new_buffer.size == 0) // nothing read
                break;
            recv_buffer_current = recv_buffer_current.slice(new_buffer.size);
            //LOG("just read " << new_buffer.size << " bytes");

            try_parse_recv_packet();
        }
    }

    void Link::try_parse_recv_packet() {
        size_t current_size = recv_buffer.as_buffer().size - recv_buffer_current.size;
        if(current_size >= sizeof(uint16_t)) {
            uint16_t expected_length = ntohs(recv_buffer.as_buffer().convert<uint16_t>(0));
            if(expected_length > recv_buffer.as_buffer().size) {
                LOG("expected_length too big");
            }
            if(expected_length <= current_size) {
                // packet is ready
                parse_recv_packet(recv_buffer.as_buffer().slice(0, expected_length));
                recv_buffer.as_buffer().delete_start(expected_length);
                recv_buffer_current = recv_buffer.as_buffer().slice(
                    recv_buffer_current.size - expected_length);
            }
        }
    }

    void Link::parse_recv_packet(Buffer data) {
        LOG("packet received: " << data << ".");
    }

    optional<Buffer> Link::recv(Buffer data) {
        return optional<Buffer>();
    }

    Link::~Link() {
        stream->close();
    }
}
