#include "reactor.h"
#include "packet_stream_util.h"
#include "misc.h"
#include "exfunctional.h"
#include "logging.h"

int main() {
    Reactor reactor;
    std::vector<FD*> fds = fd_pair(reactor);

    auto pstream = new FreePacketStream(reactor, fds[1]);

    fds[0]->set_on_read_ready([&]() {
        StackBuffer<1024> data;
        while(true) {
            Buffer read = fds[0]->read(data);
            if(read.size == 0) break;
            LOG("read " << read);
        }
    });
    fds[0]->set_on_write_ready([](){});

    pstream->set_on_recv_ready([&]() {
        while(true) {
            auto d = pstream->recv();
            if(!d) break;
            LOG("recv " << *d);
        }
    });

    pstream->set_on_send_ready([](){});

    assert(pstream->send(Buffer::from_cstr("foobar")));
    assert(fds[0]->write(Buffer::from_cstr("foobar")) == 6);

    reactor.run();
}
