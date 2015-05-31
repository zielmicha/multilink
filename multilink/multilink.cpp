#include "multilink.h"
#define LOGGER_NAME "multilink"
#include "logging.h"

namespace Multilink {
    const int QUEUE_SIZE = MULTILINK_MTU * 100;

    Multilink::Multilink(Reactor& reactor): reactor(reactor),
                                            queue(QUEUE_SIZE, MULTILINK_MTU) {

    }

    void Multilink::shuffle_links() {
        random_shuffle(links.begin(), links.end());
    }

    void Multilink::send_with_offset(const Buffer data) {
        assert(data.size <= MULTILINK_MTU);
        bool was_empty = queue.empty();
        bool pushed = queue.push_back(data);
        assert(pushed);
        if(was_empty) {
            reactor.schedule(std::bind(&Multilink::some_link_send_ready, this));
        }
    }

    bool Multilink::is_send_ready() {
        return !queue.is_full();
    }

    optional<Buffer> Multilink::recv() {
        optional<Buffer> result;
        shuffle_links();
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

        reactor.schedule(std::bind(&Multilink::link_send_ready, this, link));
        reactor.schedule(std::bind(&Multilink::link_recv_ready, this, link));

        return *link;
    }

    void Multilink::link_recv_ready(Link* link) {
        on_recv_ready();
    }

    void Multilink::link_send_ready(Link* link) {
        while(true) {
            if(queue.packet_count() < queue.max_count) {
                // queue not too full
                on_send_ready();
            }
            if(queue.empty()) {
                DEBUG("multilink queue empty");
                break;
            }

            if(link->send(++last_seq, queue.front())) {
                queue.pop_front();
            } else {
                break;
            }
        }
    }

    void Multilink::some_link_send_ready() {
        shuffle_links();
        for(auto& link: links) {
            link_send_ready(&*link);
        }
    }
}
