#include "packet_stream_util.h"
#define LOGGER_NAME "packetstreamutil"
#include "logging.h"

const int MTU = 16384;

FreePacketStream::FreePacketStream(Reactor& reactor, Stream* stream, size_t mtu):
    FreeWriterPacketStream(reactor, stream),
    recv_buffer(mtu) {
}

std::shared_ptr<FreePacketStream> FreePacketStream::create(
    Reactor& reactor, Stream* stream, size_t mtu) {
    std::shared_ptr<FreePacketStream> self {new FreePacketStream(reactor, stream, mtu)};
    stream->set_on_write_ready_and_schedule(reactor, std::bind(&FreePacketStream::write_ready, self));
    stream->set_on_read_ready_and_schedule(reactor, std::bind(&FreePacketStream::read_ready, self));
    return self;
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

// ----- FreeWriterPacketStream -----

FreeWriterPacketStream::FreeWriterPacketStream(Reactor& reactor, Stream* stream):
    reactor(reactor),
    stream(stream),
    send_buffer(MTU),
    send_buffer_current(NULL, 0) {}

bool FreeWriterPacketStream::send_with_offset(const Buffer data) {
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

void FreeWriterPacketStream::write_ready() {
    while(true) {
        if(send_buffer_current.size == 0) {
            reactor.schedule(on_send_ready);
            break;
        }
        size_t wrote = stream->write(send_buffer_current);
        if(wrote == 0) break;
        send_buffer_current = send_buffer_current.slice(wrote);
    }

    if(send_buffer_current.size == 0)
        on_send_ready();
}

// ----- LengthPacketStream -----

LengthPacketStream::LengthPacketStream(Reactor& reactor, Stream* stream):
    FreeWriterPacketStream(reactor, stream),
    recv_buffer(MTU * 2),
    recv_caller_buffer(MTU),
    recv_buffer_pos(0) {}

std::shared_ptr<LengthPacketStream> LengthPacketStream::create(
    Reactor& reactor, Stream* stream) {
    std::shared_ptr<LengthPacketStream> self {new LengthPacketStream(reactor, stream)};
    stream->set_on_write_ready_and_schedule(
        reactor, std::bind(&LengthPacketStream::write_ready, self));
    stream->set_on_read_ready_and_schedule(
        reactor, std::bind(&LengthPacketStream::read_ready, self));
    return self;
}

void LengthPacketStream::read_ready() {
    while(true) {
        Buffer data_read = stream->read(recv_buffer.as_buffer().slice(recv_buffer_pos));

        recv_buffer_pos += data_read.size;
        if(recv_buffer_pos >= 4) {
            auto expected_size = recv_buffer.as_buffer().convert<uint32_t>(0);
            if(recv_buffer_pos >= expected_size + 4) {
                on_recv_ready();
                break;
            }
        }

        if(data_read.size == 0) break;
    }
}

optional<Buffer> LengthPacketStream::recv() {
    // FIXME: copy
    if(recv_buffer_pos >= 4) {
        uint32_t expected_size = recv_buffer.as_buffer().convert<uint32_t>(0);
        assert(expected_size <= MTU);
        if(recv_buffer_pos >= expected_size + 4) {
            recv_buffer.as_buffer().slice(4, expected_size).copy_to(
                recv_caller_buffer.as_buffer());
            recv_buffer.as_buffer().delete_start(expected_size + 4, recv_buffer_pos);
            recv_buffer_pos -= expected_size + 4;

            reactor.schedule(std::bind(&LengthPacketStream::read_ready, this));

            return recv_caller_buffer.as_buffer().slice(0, expected_size);
        }
    }
    return boost::none;
}

bool LengthPacketStream::send(const Buffer data) {
    // FIXME: not optimal...
    AllocBuffer new_buffer { data.size + 4 };
    data.copy_to(new_buffer.as_buffer().slice(4));
    new_buffer.as_buffer().convert<uint32_t>(0) = data.size;

    return this->FreeWriterPacketStream::send(new_buffer.as_buffer());
}

// ----- Piping -----

void pipe(Reactor& reactor,
          std::shared_ptr<PacketStream> in, std::shared_ptr<PacketStream> out,
          size_t mtu) {
    Piper::create(reactor, in, out, mtu);
}

Piper::Piper(Reactor& reactor,
             std::shared_ptr<PacketStream> in,
             std::shared_ptr<PacketStream> out,
             size_t mtu): reactor(reactor),
                                                 in(in),
                                                 out(out),
                                                 buffer(mtu) {
}

std::shared_ptr<Piper> Piper::create(Reactor& reactor,
                                     std::shared_ptr<PacketStream> in,
                                     std::shared_ptr<PacketStream> out,
                                     size_t mtu) {
    std::shared_ptr<Piper> piper {new Piper(reactor, in, out, mtu)};
    in->set_on_recv_ready(std::bind(&Piper::recv_ready, piper));
    out->set_on_send_ready(std::bind(&Piper::send_ready, piper));
    return piper;
}

void Piper::recv_ready() {
    if(current)
        return;

    auto data = in->recv();
    if(!data)
        return;

    assert(data->size <= buffer.as_buffer().size);

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
