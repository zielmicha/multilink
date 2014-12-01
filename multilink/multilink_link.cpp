#include "multilink.h"

const int64_t SAMPLING_TIME = 2000;
const size_t MTU = 4096;

namespace Multilink {
    Link::Link(Reactor& reactor, Stream* stream): reactor(reactor),
                                                  stream(stream),
                                                  send_buffer(MTU),
                                                  send_buffer_current(NULL, 0),
                                                  bandwidth(SAMPLING_TIME) {
        stream->set_on_read_ready(std::bind(&Link::transport_read_ready, this));
        stream->set_on_write_ready(std::bind(&Link::transport_write_ready, this));
    }

    void Link::transport_write_ready() {
        if(send_buffer_current.size == 0) {
            on_send_ready();
        }
    }

    bool Link::send(Buffer data) {
        if(send_buffer_current.size != 0)
            return false; // previous packet still not sent

        if(data.size > send_buffer.as_buffer().size)
            abort();

        send_buffer_current = send_buffer.as_buffer().slice(0, data.size);
        data.copy_to(send_buffer_current);
        return true;
    }

    void Link::transport_read_ready() {
        on_recv_ready();
    }

    Link::~Link() {
        stream->close();
    }
}
