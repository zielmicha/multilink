#include "multilink.h"
#include "misc.h"
#include "logging.h"
#include "write_queue.h"

int main() {
    Reactor reactor;
    Multilink::Multilink left(reactor);
    Multilink::Multilink right(reactor);

    std::vector<FD*> fds = fd_pair(reactor);
    Timer timer(reactor);

    left.add_link(fds[0], "left");
    right.add_link(fds[1], "right");

    left.on_send_ready = nothing;
    right.on_send_ready = nothing;

    left.on_recv_ready = nothing;
    right.on_recv_ready = [&]() {
        while(true) {
            auto packet = right.recv();
            if(!packet) break;
            LOG("recv " << *packet);
        }
    };

    WriteQueue queue {reactor, &left, 10000};

    for(int i=0; i < 50; i++) {
        queue.send(Buffer::from_cstr("foobar1"));
        queue.send(Buffer::from_cstr("foobar2"));
    }

    reactor.run();
}
