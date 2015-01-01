#include "packet_stream_util.h"
#define LOGGER_NAME "packetstreamutil"
#include "logging.h"

const int MTU = 4096;

FreePacketStream::FreePacketStream(Reactor& reactor, Stream* stream): reactor(reactor),
                                                                      stream(stream),
                                                                      send_buffer(MTU),
                                                                      send_buffer_current(NULL, 0),
                                                                      recv_buffer(MTU) {
    stream->set_on_write_ready(std::bind(&FreePacketStream::write_ready, this));
    stream->set_on_read_ready(std::bind(&FreePacketStream::read_ready, this));
}

optional<Buffer> FreePacketStream::recv() {
    Buffer data = stream->read(recv_buffer.as_buffer());
    if(data.size == 0)
        return optional<Buffer>();
    else
        return data;
}

void FreePacketStream::read_ready() {
    on_recv_ready();
}

bool FreePacketStream::send(const Buffer data) {
    assert(data.size <= MTU);
    if(send_buffer_current.size == 0) {
        send_buffer_current = send_buffer.as_buffer().slice(0, data.size);
        data.copy_to(send_buffer_current);
        reactor.schedule(std::bind(&FreePacketStream::write_ready, this));
        return true;
    } else {
        return false;
    }
}

void FreePacketStream::write_ready() {
    while(true) {
        if(send_buffer_current.size == 0) {
            reactor.schedule(on_send_ready);
            break;
        }
        size_t wrote = stream->write(send_buffer_current);
        if(wrote == 0) break;
        send_buffer_current = send_buffer_current.slice(wrote);
    }
}

void pipe(Reactor& reactor, PacketStream* in, PacketStream* out) {
    new Piper(reactor, in, out);
}

Piper::Piper(Reactor& reactor, PacketStream* in, PacketStream* out): reactor(reactor),
                                                                     in(in),
                                                                     out(out),
                                                                     buffer(MTU) {
    in->set_on_recv_ready(std::bind(&Piper::recv_ready, this));
    out->set_on_send_ready(std::bind(&Piper::send_ready, this));
}

void Piper::recv_ready() {
    if(current)
        return;

    auto data = in->recv();
    if(!data)
        return;

    assert(data->size <= MTU);

    current = buffer.as_buffer().slice(0, data->size);
    data->copy_to(*current);

    reactor.schedule(std::bind(&Piper::send_ready, this));
}

void Piper::send_ready() {
    if(current) {
        if(out->send(*current)) {
            current = boost::none;
            reactor.schedule(std::bind(&Piper::recv_ready, this));
        }
    }
}
