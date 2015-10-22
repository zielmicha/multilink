#include "multilink.h"
#include "misc.h"
#include "buffer.h"
#include "logging.h"
#include "throttled.h"

using namespace Multilink;
using namespace std;

int main() {
    Reactor reactor;
    vector<FD*> fds = fd_pair(reactor);

    {
        ThrottledStream fd0(reactor, fds[0], 2);
        Link a {reactor, &fd0};
        a.name = "a";
        Link b {reactor, fds[1]};
        b.name = "b";

        int counter = 1;
        const int expcount = 500;

        a.on_send_ready = [&]() {
            if(counter < expcount) {
                char data[2000];
                memset(data, 0, sizeof(data));
                Buffer pak {data, sizeof(data)};
                auto pak_buf = AllocBuffer::copy(pak);

                if(a.send(counter, pak_buf)) {
                    counter ++;
                }
            }
            if(counter == expcount) {
                LOG("finished");
                counter ++;
            }
        };

        a.on_recv_ready = []() {
        };

        b.on_send_ready = []() {
        };

        b.on_recv_ready = [&]() {
            while(true) {
                optional<Buffer> ret = b.recv();
                if(!ret) return;
                //LOG("read packet size=" << ret->size);
            }
        };

        reactor.run();
    }
}
