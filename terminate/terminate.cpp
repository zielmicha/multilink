#define LOGGER_NAME "terminate"
#include "libreactor/logging.h"
#include "libreactor/tcp.h"
#include "libreactor/packet_stream_util.h"
#include "libreactor/bytestring.h"
#include "terminate/terminate.h"
#include "terminate/terminate_misc.h"

Terminator::Terminator(Reactor& reactor): reactor(reactor) {
}

void Terminator::tcp_accepted(TcpStreamPtr stream) {
    std::string addr = stream->get_local_address().to_string();
    int port = stream->get_local_port();
    LOG("TCP connection to " << addr << ":" << port);

    ByteString header = ByteString::copy_from("tcp\n" + addr + ":" + std::to_string(port));

    auto result_stream = std::make_shared<ReadHeaderPacketStream>(header, FreePacketStream::create(reactor, stream, transport.lock()->get_mtu()));
    transport.lock()->add_target(id_counter ++, Future<PacketStreamPtr>::make_immediate(result_stream));
}

TerminatorPtr Terminator::create(Reactor& reactor, bool is_server) {
    TerminatorPtr self (new Terminator(reactor));
    self->is_server = is_server;
    if (!is_server)
        self->id_counter = 100;
    else
        self->id_counter = (1 << 1ull) + 100;
    return self;
}

Future<PacketStreamPtr> Terminator::create_target(uint64_t id) {
    if (((id & (1ull << 63)) != 0) != !is_server) {
        LOG("requested stream with an unexpected id " << id);
        return Future<PacketStreamPtr>::make_exception("bad stream ID");
    }

    return Future<PacketStreamPtr>::make_immediate(std::make_shared<HeaderPacketStream>(std::bind(&Terminator::create_target_2, shared_from_this(), std::placeholders::_1)));
}

Future<PacketStreamPtr> Terminator::create_target_2(Buffer header) {
    std::string header_string = header;
    int type_len = header_string.find("\n");
    if (type_len == -1) type_len = header_string.size();
    std::string type = header_string.substr(0, type_len);
    std::string rest = header_string.substr(type_len + 1, header_string.size());

    if (type == "tcp") {
        int pos = rest.find(":");
        if (pos == -1) {
            return Future<PacketStreamPtr>::make_exception("bad tcp address format");
        }
        int port;
        try {
            port = stoi(rest.substr(pos + 1, rest.size() - pos - 1));
        } catch(std::exception) {
            return Future<PacketStreamPtr>::make_exception("bad tcp port format");
        }
        std::string addr = rest.substr(0, pos);
        return TCP::connect(reactor, addr, port, bind_address).then([this](StreamPtr stream) -> PacketStreamPtr {
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

    network_interface = std::make_shared<NetworkInterface>(reactor);
    network_interface->set_on_send_packet([this](IpAddress addr, Buffer buf) {
        this->tun.lock()->send(buf);
    });
    tcp_listener = std::make_shared<TcpListener>(reactor);
    tcp_listener->on_accept = std::bind(&Terminator::tcp_accepted, this, std::placeholders::_1);

    tun->set_on_recv_ready(std::bind(&Terminator::tun_recv_ready, shared_from_this()));
}

void Terminator::tun_recv_ready() {
    while (true) {
        auto packet = tun.lock()->recv();
        if (!packet) break;

        network_interface->on_recv(*packet);
    }
}
