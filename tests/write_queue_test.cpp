#include "multilink.h"
#include "reactor.h"
#include "write_queue.h"
#include "packet_stream_util.h"
#include "misc.h"
#include "logging.h"

int main() {
    Reactor reactor;

    std::vector<FD*> fds = fd_pair(reactor);
    Timer timer(reactor);

    fds[1]->on_read_ready = [&]() {
        std::cout << "on_read_ready" << std::endl;
        uint64_t size = 0;
        while(true) {
            StackBuffer<10> buff;
            Buffer data = fds[1]->read(buff);
            size += data.size;
            std::cout << size << " read " << data << std::endl;
            if(data.size == 0) break;
        }
    };

    auto left_packet = FreePacketStream::create(reactor, fds[0]);
    auto queue = WriteQueue::create(reactor, left_packet, 10000);
    for(int i=0; i < 100; i++) {
        assert(queue->send(Buffer::from_cstr("Foobar1")));
        assert(queue->send(Buffer::from_cstr("Foobar2")));
    }

    reactor.run();

}
