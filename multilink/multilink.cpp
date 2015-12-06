#include "multilink/multilink.h"
#define LOGGER_NAME "multilink"
#include "libreactor/logging.h"

namespace Multilink {
    const int QUEUE_SIZE = 40;

    Multilink::Multilink(Reactor& reactor): reactor(reactor) {

    }

    void Multilink::shuffle_links() {
        random_shuffle(links.begin(), links.end());
    }

    void Multilink::send_with_offset(const Buffer data) {
        assert(data.size <= MULTILINK_MTU);
        bool was_empty = queue.empty();
        assert(queue.size() < QUEUE_SIZE); // TODO
        queue.push_back(AllocBuffer::copy(data));
        if(was_empty) {
            LOG("queue was empty!");
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
            if(result) {
                LOG("recv from " << (*link) << " " << (uint64_t)link->rtt.mean() << " id " << (result->convert<uint64_t>(8))
                    << " channel " << (result->convert<uint64_t>(0)));
                return result;
            }
        }
        return result;
    }

    Link& Multilink::add_link(StreamPtr stream, std::string name) {
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
            if(queue.size() < QUEUE_SIZE * 3 / 4) {
                // queue not too full
                on_send_ready();
            }

            auto& own_queue = assigned_packets[link];
            if(own_queue.size() == 0) {
                request_packets(link);
            }

            if(own_queue.size() == 0)
                break;

            if(link->send(++last_seq, own_queue.front())) {
                own_queue.pop_front();
            } else {
                break;
            }
        }
    }

    void Multilink::request_packets(Link* link) {
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
        /**
         * Assign packets to links (at least) until there is a packet
         * assigned to link `until`.
         */
        if(links.empty()) return;

        DEBUG("assign " << queue.size());

        uint64_t max_time = 0;

        std::vector<uint64_t> estimated;
        std::vector<uint64_t> num_assigned;

        for(size_t i = 0; i < links.size(); i ++) {
            num_assigned.push_back(0);

            uint64_t estimated_size = links[i]->get_estimated_in_flight();

            DEBUG("estimated in flight " << estimated_size << " for " << *links[i]);

            for(auto& pkt : assigned_packets[&*links[i]]) // TODO: O(1)
                estimated_size += pkt.as_buffer().size;

            estimated.push_back(estimated_size);
            DEBUG("estimated size " << estimated_size << " for " << *links[i]);
        }

        auto key = [&](int i) -> uint64_t {
            const int WEIGHT = 2;
            uint64_t ret = links[i]->rtt.mean() * WEIGHT +
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

        int drain_to;
        if(queue.size() < QUEUE_SIZE * 2 / 3)
            drain_to = 0;
        else
            drain_to = queue.size() / 2;

        while(queue.size() > drain_to) {
            AllocBuffer packet {std::move(queue.front())};
            queue.pop_front();
            size_t packet_size = packet.as_buffer().size;

            int link = *L.begin();
            Link* link_ptr = &*(links[link]);

            std::pop_heap(L.begin(), L.end(), cmp);

            DEBUG("assign to " << *links[link]);

            estimated[link] += packet_size;
            max_time = std::max(max_time, key(link));

            num_assigned[link] ++;
            assigned_packets[link_ptr].push_back(std::move(packet));

            //if(link_ptr == until)
            //    break;

            std::push_heap(L.begin(), L.end(), cmp);
        }

        DEBUG("assigned max time " << max_time);
        for(size_t i = 0; i < links.size(); i ++) {
            LOG("assigned " << num_assigned[i] << " to " << *links[i]);
        }
    }
}
