#define LOGGER_NAME "ippacket"
#include "libreactor/logging.h"
#include "terminate/ippacket.h"

const int MAX_TUN_QUEUE_SIZE = 64;

RawPacketStream::RawPacketStream(bool is_server): is_server(is_server), output_buffer(4096) {
}

void RawPacketStream::set_tun(TunPtr tun, std::string ip) {
    this->tun = tun;
    this->ip4 = IpAddress::parse4(ip);
}

struct Protocol {
    enum {
        ICMP = 1, TCP = 6, UDP = 17, ICMP6 = 58
    };
};

bool is_valid_protocol(int p) {
    return p == Protocol::ICMP || p == Protocol::UDP || p == Protocol::ICMP6;
}


bool RawPacketStream::is_send_ready() {
    return true;
}

bool RawPacketStream::prepare_tun_packet(Buffer buffer) {
    buffer.convert<uint16_t>(0) = 0;

    Buffer packet = buffer.slice(4);

    if (packet.size < 20) {
        LOG ("short packet (" << packet.size << " bytes) received from stream");
        return false;
    }

    int version = packet.convert<uint8_t>(0) >> 4;

    if (version == 4) {
        if (is_server) {
            // overwrite src address
            packet.convert<uint32_t>(12) = ip4.addr4;
        } else {
            // overwrite dst address
            packet.convert<uint32_t>(16) = ip4.addr4;
        }

        buffer.convert<uint16_t>(2) = htons(0x0800);
        return true;
    } else if (version == 6) {
        buffer.convert<uint16_t>(2) = htons(0x86DD);
        return true;
    } else {
        LOG ("unknown IP version");
        return false;
    }
}

void RawPacketStream::send(const Buffer _data) {
    // TODO: if (is_server) - check incoming packets
    auto tun_ptr = tun.lock();

    if (_data.size > output_buffer.size - 4) {
        LOG ("packet too big");
        return;
    }

    Buffer packet = output_buffer.as_buffer().slice(0, _data.size + 4);
    packet.slice(4).copy_from(_data);

    if (!prepare_tun_packet(packet)) return;

    LOG ("output on wire packet " << packet);
    if (tun_ptr)
        tun_ptr->send(packet);
}

optional<Buffer> RawPacketStream::recv() {
    if (from_tun_queue.empty()) {
        return optional<Buffer>();
    } else {
        from_tun_current = std::move(from_tun_queue.front());
        from_tun_queue.pop_front();
        return from_tun_current.as_buffer();
    }
}

bool RawPacketStream::check_udp(Buffer payload) {
    if (is_server) return true;
    if (payload.size < 8) {
        LOG ("truncated UDP packet");
        return false;
    }

    int dst_port = ntohs(payload.convert<uint16_t>(2));
    if (dst_port == 443) {
        // Prevent QUIC sessions from being established (TCP is better in this case)
        return false;
    }

    return true;
}

bool RawPacketStream::from_tun(Buffer packet) {
    if (packet.size < 20) {
        LOG ("short packet (" << packet.size << " bytes) received from tun");
        return false;
    }

    int version = packet.convert<uint8_t>(0) >> 4;

    if (version == 4) {
        int ihl = (packet.convert<uint8_t>(0) & 0xF);
        int header_length = ihl * 4;
        if (packet.size < header_length) {
            LOG ("truncated packet");
            return false;
        }
        int protocol = packet.convert<uint8_t>(9);

        if (protocol == Protocol::TCP) return false;
        if (!is_valid_protocol(protocol)) {
            LOG ("invalid protocol " << protocol);
            return false;
        }
        if (protocol == Protocol::UDP) {
            if (!check_udp(packet.slice(header_length))) {
                LOG ("dropping UDP packet");
                return true;
            }
        }
    } else {
        LOG ("unknown IP version");
        return false;
    }

    if (from_tun_queue.size() <= MAX_TUN_QUEUE_SIZE)
        from_tun_queue.push_back(std::move(AllocBuffer::copy(packet)));

    return true;
}
