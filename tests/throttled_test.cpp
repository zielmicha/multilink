#include "throttled.h"
#include "misc.h"
#include "streamutil.h"
#include "logging.h"

void write_test(Reactor& reactor, Stream* out, int count);

int main() {
    Reactor reactor;
    std::vector<FD*> fds = fd_pair(reactor);

    {
        Stream* out = new ThrottledStream(reactor, fds[0], 0.00001);
        Stream* in = fds[1];

        out->set_on_read_ready(nothing);
        in->set_on_write_ready(nothing);

        read_all(in, [](Buffer data) {
            LOG("read " << data);
        });

        write_test(reactor, out, 100);

        reactor.run();
    }
}

void write_test(Reactor& reactor, Stream* out, int count) {
    if(count < 0) return;
    write_all(reactor, out, Buffer::from_cstr("foo")).on_success([count, &reactor, out](unit _) {
        write_test(reactor, out, count - 1);
    });
}
