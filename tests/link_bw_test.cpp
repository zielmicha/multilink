#include "multilink.h"
#include "misc.h"
#include "buffer.h"
#include "logging.h"
using namespace Multilink;
using namespace std;

int main() {
    Reactor reactor;
    vector<FD*> fds = fd_pair(reactor);

    {
        Link a {reactor, fds[0]};
        a.name = "a";
        Link b {reactor, fds[1]};
        b.name = "b";

        int counter = 1;

        a.on_send_ready = [&]() {
            if(counter != 100) {
                char data[2000];
                memset(data, 0, sizeof(data));
                Buffer pak {data, sizeof(data)};

                LOG("send " << counter);
                if(a.send(counter, pak)) {
                    counter ++;
                }
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
                LOG("read packet");
            }
        };

        reactor.run();
    }
}
