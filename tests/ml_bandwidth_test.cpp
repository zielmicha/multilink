#include "multilink/multilink.h"
#include "libreactor/misc.h"
#include "libreactor/logging.h"
#include "multilink/write_queue.h"
#include "libreactor/throttled.h"

std::pair<Stream*, FD*> create_throttled_pair(Reactor& reactor, double speed, uint64_t buffsize, uint64_t delay) {
    std::vector<FD*> fds = fd_pair(reactor);
    Stream* delayed = new DelayedStream(reactor, fds[0], buffsize, delay);
    return {new ThrottledStream(reactor, delayed, speed), fds[1]};
}

int main() {
    Reactor reactor;
    Multilink::Multilink left(reactor);
    Multilink::Multilink right(reactor);

    Timer timer(reactor);

    auto pair1 = create_throttled_pair(reactor, 2, 100000, 1000);
    auto pair2 = create_throttled_pair(reactor, 3, 100000, 100000);

    left.add_link(pair1.first, "left1");
    right.add_link(pair1.second, "right1");

    left.add_link(pair2.first, "left2");
    right.add_link(pair2.second, "right2");

    left.on_send_ready = nothing;
    right.on_send_ready = nothing;

    left.on_recv_ready = nothing;
    right.on_recv_ready = [&]() {
        while(true) {
            auto packet = right.recv();
            if(!packet) break;
        }
    };

    write_null_packets(&left, 1024);

    reactor.run();
}
