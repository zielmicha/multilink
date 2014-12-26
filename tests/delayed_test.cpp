#include "throttled.h"
#include "misc.h"
#include "streamutil.h"
#include "logging.h"


int main() {
    Reactor reactor;
    std::vector<FD*> fds = fd_pair(reactor);

    {
        Stream* out = new DelayedStream(reactor, fds[0], 100, 1000 * 1000);
        Stream* in = fds[1];

        out->set_on_read_ready(nothing);
        in->set_on_write_ready(nothing);

        bool second = false;

        out->set_on_write_ready([&]() {
            LOG("write ready");
            if(!second) {
                second = true;
                assert(out->write(Buffer::from_cstr("bazbar")) == 6);
            }
        });

        read_all(in, [](Buffer data) {
            LOG("read " << data);
        });

        Buffer data = Buffer::from_cstr("foobar");

        while(true) {
            size_t s = out->write(data);
            LOG("write " << s);
            if(s != data.size) break;
        }

        reactor.run();
    }
}
