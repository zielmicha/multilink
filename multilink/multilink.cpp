#include "multilink.h"
#define LOGGER_NAME "multilink"
#include "logging.h"

namespace Multilink {
    const int QUEUE_SIZE = MULTILINK_MTU * 100;

    Multilink::Multilink(Reactor& reactor): reactor(reactor) {

    }

    void Multilink::shuffle_links() {
        random_shuffle(links.begin(), links.end());
    }

    void Multilink::send_with_offset(const Buffer data) {
        assert(data.size <= MULTILINK_MTU);
        bool was_empty = queue.empty();
        assert(queue.size() < QUEUE_SIZE);
        queue.push_back(AllocBuffer::copy(data));
        if(was_empty) {
            reactor.schedule(std::bind(&Multilink::some_link_send_ready, this));
        }
    }

    bool Multilink::is_send_ready() {
        return queue.size() < QUEUE_SIZE;
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
            auto& own_queue = assigned_packets[link];
            if(own_queue.size() == 0)
                request_packets(link);

            if(own_queue.size() == 0)
                break;

            if(link->send(++last_seq, own_queue.front().as_buffer())) {
                own_queue.pop_front();
            } else {
                break;
            }
        }
    }

    void Multilink::request_packets(Link* link) {
        if(queue.size() < QUEUE_SIZE) {
            // queue not too full
            on_send_ready();
        }

        if(queue.empty()) {
            DEBUG("multilink queue empty");
            return;
        }

        assign_links_until(link);
    }

    void Multilink::some_link_send_ready() {
        shuffle_links();
        for(auto& link: links) {
            link_send_ready(&*link);
        }
    }

    void Multilink::assign_links_until(Link* until) {
        if(links.empty()) return;

        std::vector<uint64_t> estimated;

        for(size_t i = 0; i < links.size(); i ++) {
            estimated.push_back(0) /* links[i].get_estimated_in_flight() */; // TODO
        }

        auto key = [&](int i) -> uint64_t {
            uint64_t ret = links[i]->rtt.mean() +
            estimated[i] / links[i]->bandwidth.bandwidth_mbps();
            DEBUG(*links[i] << " -> " << ret);
            return ret;
        };

        auto cmp = [&](int i, int j) -> bool {
            return key(i) > key(j);
        };

        std::vector<int> L;
        for(size_t i = 0; i < links.size(); i ++)
            L.push_back(i);

        std::make_heap(L.begin(), L.end(), cmp);

        while(!queue.empty()) {
            AllocBuffer packet {std::move(queue.front())};
            queue.pop_front();
            size_t packet_size = packet.as_buffer().size;

            int link = *L.begin();
            Link* link_ptr = &*(links[link]);

            std::pop_heap(L.begin(), L.end(), cmp);

            LOG("assign to " << *links[link]);

            estimated[link] += packet_size;
            assigned_packets[link_ptr].push_back(std::move(packet));

            if(link_ptr == until)
                break;

            std::push_heap(L.begin(), L.end(), cmp);
        }
    }
}
