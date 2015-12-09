#include "multilink/transport.h"
#define LOGGER_NAME "transport"
#include "libreactor/logging.h"

// MAX_BUFFER_SIZE - BUFFER_SIZE_MARK has to be bigger than bandwidth-delay product
//const int MAX_BUFFER_SIZE = 8 * 1024 * 1024;
const int MAX_BUFFER_PACKETS = 16 * 1024;
//const int BUFFER_SIZE_LOW_MARK = 256 * 1024;
//const int BUFFER_SIZE_HIGH_MARK = 512 * 1024;

const uint64_t LAST_CHANNEL = std::numeric_limits<uint64_t>::max();
//const uint64_t CHOKE_CHANNEL = LAST_CHANNEL;
//const uint64_t UNCHOKE_CHANNEL = LAST_CHANNEL - 1;
const uint64_t CLOSE_CHANNEL = LAST_CHANNEL - 2;

std::shared_ptr<Transport> Transport::create(
    Reactor& reactor,
    std::shared_ptr<PacketStream> network_stream,
    TargetCreator target_creator, size_t mtu) {
    std::shared_ptr<Transport> transport {new Transport(network_stream, target_creator, mtu)};

    reactor.schedule(std::bind(&Transport::network_send_ready, transport));
    reactor.schedule(std::bind(&Transport::network_recv_ready, transport));
    network_stream->set_on_send_ready(std::bind(&Transport::network_send_ready, transport));
    network_stream->set_on_recv_ready(std::bind(&Transport::network_recv_ready, transport));

    return transport;
}

Transport::ChildStream::ChildStream() {
}


std::shared_ptr<Transport::ChildStream> Transport::get_child_stream(uint64_t id) {
    auto it = children.find(id);
    if(it == children.end()) {
        LOG("create child");
        create_child_stream(id);
        return children.find(id)->second;
    } else {
        return it->second;
    }
}

void Transport::create_child_stream(uint64_t id) {
    add_target(id, target_creator(id));
}

void Transport::add_target(uint64_t id, Future<std::shared_ptr<PacketStream> > target) {
    std::shared_ptr<ChildStream> stream {new ChildStream()};
    stream->id = id;
    stream->transport = shared_from_this();
    children[id] = stream;

    auto shared_this = shared_from_this();

    target.then<unit>([shared_this, stream](std::shared_ptr<PacketStream> target) -> unit {
        assert (target);

        stream->target = target;

        target->set_on_send_ready(std::bind(&Transport::target_send_ready, shared_this, stream));
        target->set_on_recv_ready(std::bind(&Transport::target_recv_ready, shared_this, stream));
        target->set_on_error(std::bind(&Transport::target_error, shared_this, stream));

        shared_this->target_send_ready(stream);
        shared_this->target_recv_ready(stream);

        return {};
    }).ignore();
}

void Transport::target_send_ready(std::shared_ptr<ChildStream> child) {
    DEBUG("target_send_ready");
    if(!child->target) {
        LOG("early return from target_send_ready - target closed");
    }

    while(!child->buffer.empty() && !child->buffer.front().empty()) {
        if(child->target && child->target->is_send_ready()) {
            child->target->send(child->buffer.front().as_buffer());
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

        if(!Transport::target_recv_ready_once(child)) break;
    }
}

bool Transport::target_recv_ready_once(std::shared_ptr<ChildStream> child) {
    if(!child->target)
        return false;

    optional<Buffer> packet = child->target->recv();
    if(packet) {
        AllocBuffer new_buffer_alloc {packet->size + 2 * sizeof(uint64_t)}; // TODO: use single member buffer
        Buffer new_buffer = new_buffer_alloc.as_buffer();

        new_buffer.convert<uint64_t>(0) = child->id;
        new_buffer.convert<uint64_t>(8) = child->last_sent_packet;
        child->last_sent_packet ++;
        new_buffer.slice(16).copy_from(*packet);

        DEBUG("network send " << new_buffer);
        network_stream->send(new_buffer);
        return true;
    } else {
        return false;
    }
}

void Transport::target_error(std::shared_ptr<ChildStream> child) {
    LOG("target error");
    special_packet_queue.push_back({CLOSE_CHANNEL, child->id});
    send_special_packet();

    child->target->close();
    child->target = nullptr;
}

void Transport::close_channel(uint64_t id) {
    auto child = get_child_stream(id);

    LOG("close_channel " << id);

    if(child->target) {
        child->target->close();
        child->target = nullptr;
    }
}

void Transport::send_special_packet() {
    if(!network_stream->is_send_ready()) return;

    if(!special_packet_queue.empty()) {
        auto packet = special_packet_queue.front();
        StackBuffer<16> buffer;
        Buffer(buffer).convert<uint64_t>(0) = packet.first;
        Buffer(buffer).convert<uint64_t>(8) = packet.second;
        special_packet_queue.pop_front();
        network_stream->send(buffer);
        LOG("special packet " << int64_t(packet.first) << " " << packet.second << " sent");
    }
}

void Transport::network_recv_ready() {
    while(true) {
        optional<Buffer> packet = network_stream->recv();
        if(packet) {
            uint64_t id = packet->convert<uint64_t>(0);
            uint64_t seq = packet->convert<uint64_t>(8);

            if(id == CLOSE_CHANNEL) {
                close_channel(seq);
            } else {
                place_packet(id, seq, packet->slice(16));
            }
        } else {
            return;
        }
    }
}

void Transport::place_packet(uint64_t id, uint64_t seq, Buffer data) {
    DEBUG("network recv " << id << " " << seq << " " << data);
    auto child = get_child_stream(id);

    int64_t rel_seq_64 = int64_t(seq) - child->last_forwarded_packet;

    if(rel_seq_64 < 0) {
        LOG("old dup!");
        return;
    }

    if(rel_seq_64 >= MAX_BUFFER_PACKETS) {
        // TODO: close connection instead of failing
        assert(false);
    }

    int rel_seq = rel_seq_64;

    // TODO: check if MAX_QUEUE_SIZE is not exceeded
    // TODO: choke/unchoke

    if(((int)child->buffer.size()) < rel_seq + 1) {
        LOG("resize to " << rel_seq + 1);
        child->buffer.resize(rel_seq + 1);
    }

    if(!child->buffer[rel_seq].empty()) {
        // duplicate
        LOG("dup!");
        return;
    }

    child->buffer[rel_seq] = AllocBuffer::copy(data);

    if(rel_seq == 0) {
        // now it's possible to send
        target_send_ready(child);
    }
}

void Transport::network_send_ready() {
    DEBUG("network_send_ready");
    while(true) {
        bool anything = false;

        if(!network_stream->is_send_ready()) {
            DEBUG("not longer send ready - exit");
            return;
        }

        if(!special_packet_queue.empty()) {
            anything = true;
            send_special_packet();
            continue;
        }

        for(auto entry: children) {
            if(!network_stream->is_send_ready()) return;

            if(target_recv_ready_once(entry.second))
                anything = true;
        }
        if(!anything) {
            DEBUG("no stream has written anything - exit");
            return;
        }
    }
}
