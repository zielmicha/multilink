#include "reorder_stream.h"
#include "multilink.h"
#include "reactor.h"
#include "misc.h"
#include "logging.h"

int main() {
    Reactor reactor;
    Multilink::Multilink left(reactor);
    Multilink::Multilink right(reactor);

    std::vector<FD*> fds = fd_pair(reactor);
    Timer timer(reactor);

    left.add_link(fds[0], "left");
    right.add_link(fds[1], "right");

    right.on_send_ready = nothing;
    right.on_recv_ready = nothing;

    ReorderStream left_reorder {&left};

    auto send_packet = [&](uint64_t seq, char data) {
        StackBuffer<9> stack_buff;
        Buffer buff = stack_buff;
        buff.data[8] = data;
        buff.convert<uint64_t>(0) = seq;
        assert(right.send(buff));
    };

    send_packet(1, 'a');
    send_packet(3, 'c');
    send_packet(3, 'c');
    send_packet(2, 'b');
    send_packet(1, 'a');
    send_packet(5, 'e');

    left_reorder.on_recv_ready = [&]() {
        LOG("recv ready");
        while(true) {
            auto packet = left_reorder.recv();
            if(!packet) break;
            LOG("recv " << *packet);
        }
    };
    left_reorder.on_send_ready = nothing;

    timer.once(1000 * 1000, [&]() {
        LOG("send d");
        send_packet(4, 'd');
    });

    reactor.run();

}
