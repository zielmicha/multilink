#include "transport.h"
#include "logging.h"
#include "write_queue.h"
#include "timer.h"

#include "packet_test_util.h"

int main() {
    Reactor reactor;
    Timer timer {reactor};

    std::vector<std::shared_ptr<PacketStream> > network = packet_stream_pair(reactor);
    std::vector<std::shared_ptr<PacketStream> > target = packet_stream_pair(reactor);

    TargetCreator creator = [&](uint64_t id) -> Future<std::shared_ptr<PacketStream> > {
        assert(id == 0);
        LOG("create");
        return make_future(target[1]);
    };
    auto transport = Transport::create(reactor, network[1], creator, 4096);

    target[1]->set_on_send_ready(nothing);
    target[1]->set_on_recv_ready(nothing);

    target[0]->set_on_send_ready(nothing);
    network[0]->set_on_send_ready(nothing);

    packet_printer(reactor, network[0], "network");
    packet_printer(reactor, target[0], "target");

    auto queue = WriteQueue::create(reactor, network[0], 4096 * 100);
    auto target_queue = WriteQueue::create(reactor, target[0], 4096 * 100);

    int l = 10;
    for(int i=0; i < l; i ++) {
        AllocBuffer ab(17);
        Buffer b = ab.as_buffer();
        b.convert<uint64_t>(0) = 0;
        b.convert<uint64_t>(8) = l - i - 1;
        b.data[16] = 'a' + i;
        assert(queue->send(b));
    }

    timer.once(100 * 1000, [&]() {
        for(int i : {0, 1, 4, 4, 3, 1, 2}) {
            AllocBuffer ab(17);
            Buffer b = ab.as_buffer();
            b.convert<uint64_t>(0) = 0;
            b.convert<uint64_t>(8) = l + i;
            b.data[16] = 'A' + i;
            assert(queue->send(b));
            assert(queue->send(b));
        }

        target_queue->send(Buffer::from_cstr("foobar"));
        target_queue->send(Buffer::from_cstr("AA"));
    });

    reactor.run();
}
