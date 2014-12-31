#include "packet_stream_util.h"
const int MTU = 4096;

class PipeStreamToPacketStream {
public:
    AllocBuffer buff;
    Buffer data;
    Stream* in;
    PacketStream* out;

    PipeStreamToPacketStream(Stream* in, PacketStream* out): buff(MTU),
                                                             data(NULL, 0),
                                                             in(in),
                                                             out(out) {
        out->set_on_send_ready(std::bind(&PipeStreamToPacketStream::on_send_ready, this));
        in->set_on_read_ready(std::bind(&PipeStreamToPacketStream::on_read_ready, this));
    }

    void on_read_ready() {
        if(data.size == 0) {
            data = in->read(buff.as_buffer());
            if(data.size != 0)
                on_send_ready();
        }
    }

    void on_send_ready() {
        if(data.size != 0) {
            if(out->send(data))
                data = data.slice(0, 0);
        } else {
            on_read_ready();
        }
    }
};

void pipe(Stream* in, PacketStream* out) {
    new PipeStreamToPacketStream(in, out);
}


class PipePacketStreamToStream {
public:
    AllocBuffer buff;
    Buffer data;
    PacketStream* in;
    Stream* out;

    PipePacketStreamToStream(PacketStream* in, Stream* out): buff(MTU),
                                                             data(NULL, 0),
                                                             in(in),
                                                             out(out) {
        in->set_on_recv_ready(std::bind(&PipePacketStreamToStream::on_recv_ready, this));
        out->set_on_write_ready(std::bind(&PipePacketStreamToStream::on_write_ready, this));
    }

    void on_recv_ready() {
        if(data.size == 0) {
            auto r = in->recv();
            if(!r) return;
            if(r->size > MTU) abort();
            data = buff.as_buffer().slice(0, r->size);
            r->copy_to(data);
            on_write_ready();
        }
    }

    void on_write_ready() {
        while(data.size != 0) {
            size_t bytes = out->write(data);
            if(bytes == 0) break;
            data = data.slice(bytes);
        }
        if(data.size == 0) {
            on_recv_ready();
        }
    }
};

void pipe(PacketStream* in, Stream* out) {
    new PipePacketStreamToStream(in, out);
}
