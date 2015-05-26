#include "transport.h"

// MAX_BUFFER_SIZE - BUFFER_SIZE_MARK has to be bigger than bandwidth-delay product
const int MAX_BUFFER_SIZE = 8 * 1024 * 1024;
const int BUFFER_SIZE_LOW_MARK = 256 * 1024;
const int BUFFER_SIZE_HIGH_MARK = 512 * 1024;

const uint64_t CHOKE_CHANNEL = std::numeric_limits<uint64_t>::max();
const uint64_t UNCHOKE_CHANNEL = CHOKE_CHANNEL - 1;

std::shared_ptr<Transport> Transport::create(std::shared_ptr<PacketStream> network_stream,
                                             TargetCreator target_creator, size_t mtu) {
    std::shared_ptr<Transport> transport {new Transport(network_stream, target_creator, mtu)};

    network_stream->set_on_send_ready(std::bind(&Transport::network_send_ready, transport));
    network_stream->set_on_recv_ready(std::bind(&Transport::network_recv_ready, transport));

    return transport;
}

Transport::ChildStream::ChildStream() {
}


void Transport::create_child_stream(uint64_t id) {
    std::shared_ptr<ChildStream> stream {new ChildStream()};
    stream->id = id;
    stream->transport = shared_from_this();
    children[id] = stream;

    auto shared_this = shared_from_this();

    target_creator(id).then<unit>([shared_this, stream](std::shared_ptr<PacketStream> target) -> unit {
        stream->target = target;

        target->set_on_send_ready(std::bind(&Transport::target_send_ready, shared_this, stream));
        target->set_on_recv_ready(std::bind(&Transport::target_recv_ready, shared_this, stream));

        return {};
    });
}

void Transport::target_send_ready(std::shared_ptr<ChildStream> child) {
    while(!child->buffer.empty() && !child->buffer.front().empty()) {
        if(child->target->send(child->buffer.front().as_buffer())) {
            child->buffer.pop_front();
            child->last_forwarded_packet ++;
        } else {
            break;
        }
    }
    // TODO: unchoke if choked and buffer_size below BUFFER_SIZE_LOW_MARK
}

void Transport::target_recv_ready(std::shared_ptr<ChildStream> child) {
    while(true) {
        if(!network_stream->is_send_ready()) break;
        optional<Buffer> packet = child->target->recv();
        if(!packet) break;
        bool ok = network_stream->send(*packet);
        assert(ok);
    }
}
