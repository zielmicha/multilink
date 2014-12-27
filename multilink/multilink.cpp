#include "multilink.h"

const int QUEUE_SIZE = 100000;

namespace Multilink {
    Multilink::Multilink(Reactor& reactor): reactor(reactor),
                                            queue(QUEUE_SIZE) {

    }

    bool Multilink::send(const Buffer data) {
        return queue.push_back(data);
    }

    optional<Buffer> Multilink::recv() {
        optional<Buffer> result;
        for(auto& link: links) {
            result = link->recv();
            if(result) return result;
        }
        return result;
    }

    Link& Multilink::add_link(Stream* stream, std::string name) {
        links.emplace_back(new Link(reactor, stream));

        Link* link = &(*links.back());

        link->on_recv_ready = std::bind(&Multilink::link_recv_ready, this, link);
        link->on_send_ready = std::bind(&Multilink::link_send_ready, this, link);
        link->name = name;
        return *link;
    }

    void Multilink::link_recv_ready(Link* link) {
        on_recv_ready();
    }

    void Multilink::link_send_ready(Link* link) {

    }
}
