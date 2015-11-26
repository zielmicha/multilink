#define LOGGER_NAME "terminate"
#include "libreactor/logging.h"
#include "libreactor/tcp.h"
#include "libreactor/packet_stream_util.h"
#include "terminate/terminate.h"

class HeaderPacketStream : public AbstractPacketStream, public std::enable_shared_from_this<HeaderPacketStream> {
    typedef std::function<Future<PacketStreamPtr>(Buffer)> Callback;
    Callback callback;
    PacketStreamPtr new_stream = nullptr;

public:
    HeaderPacketStream(Callback callback): callback(callback) {}

    void set_on_recv_ready(std::function<void()> f) {
        if (new_stream) new_stream->set_on_recv_ready(f);
        AbstractPacketStream::set_on_recv_ready(f);
    }

    void set_on_send_ready(std::function<void()> f) {
        if (new_stream) new_stream->set_on_send_ready(f);
        AbstractPacketStream::set_on_send_ready(f);
    }

    void set_on_error(std::function<void()> f) {
        if (new_stream) new_stream->set_on_error(f);
        AbstractPacketStream::set_on_error(f);
    }

    size_t get_send_offset() {
        return 0;
    }

    void close() { // FIXME: race condition
        if (new_stream)
            new_stream->close();
    }

    void send(Buffer data) {
        if (new_stream) {
            new_stream->send(data);
        } else {
            auto self = this->shared_from_this();
            callback(data).then([self](PacketStreamPtr stream) -> unit {
                self->new_stream = stream;
                self->on_send_ready();
                self->on_recv_ready();
                return {};
            }).ignore();
            callback = nullptr;
        }
    }

    void send_with_offset(Buffer data) {
        send(data);
    }

    bool is_send_ready() {
        if (new_stream)
            return new_stream->is_send_ready();
        else
            return callback ? true : false;
    }

    optional<Buffer> recv() {
        if (new_stream)
            return new_stream->recv();
        else
            return optional<Buffer>();
    }
};

Terminator::Terminator(Reactor& reactor): reactor(reactor) {

}

TerminatorPtr Terminator::create(Reactor& reactor, bool is_server) {
    TerminatorPtr self (new Terminator(reactor));
    self->is_server = is_server;
    return self;
}

Future<PacketStreamPtr> Terminator::create_target(uint64_t id) {
    if ((bool)(id & (1ull << 63)) == !is_server) {
        LOG("requested stream with an unexpected id " << id);
        return Future<PacketStreamPtr>::make_immediate(PacketStreamPtr(nullptr));
    }

    return Future<PacketStreamPtr>::make_immediate(PacketStreamPtr
                                                   (new HeaderPacketStream(std::bind(&Terminator::create_target_2, shared_from_this(), std::placeholders::_1))));
}

Future<PacketStreamPtr> Terminator::create_target_2(Buffer header) {
    std::string header_string = header;
    int type_len = header_string.find("\n");
    if (type_len == -1) type_len = header_string.size();
    std::string type = header_string.substr(0, type_len);
    std::string rest = header_string.substr(type_len, header_string.size() - type_len);

    if (type == "tcp") {
        int pos = rest.find(":");
        if (pos == -1) {
            return Future<PacketStreamPtr>::make_exception("bad tcp address format");
        }
        int port = atoi(rest.substr(pos + 1, rest.size() - pos - 1).c_str()); // TODO: error detection?
        std::string addr = rest.substr(0, pos);
        return TCP::connect(reactor, addr, port).then([this](StreamPtr stream) -> PacketStreamPtr {
            return FreePacketStream::create(reactor, stream);
        });
    } else {
        LOG("unknown stream type " << type);
        return Future<PacketStreamPtr>::make_exception("unknown stream type");
    }
}

void Terminator::set_transport(TransportPtr transport) {
    this->transport = transport;
}

void Terminator::set_tun(TunPtr tun) {
    this->tun = tun;
    tun->on_recv = std::bind(&Terminator::tun_recv, shared_from_this(), std::placeholders::_1);
}

void Terminator::tun_recv(Buffer buffer) {

}
