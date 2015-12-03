#include "multilink/multilink.h"
#include "libreactor/misc.h"
#include "libreactor/buffer.h"
#include "libreactor/logging.h"
using namespace Multilink;
using namespace std;

int main() {
    Reactor reactor;
    vector<FDPtr> fds = fd_pair(reactor);

    {
        Link a {reactor, fds[0]};
        a.name = "a";
        Link b {reactor, fds[1]};
        b.name = "b";

        int counter = 10;

        a.on_send_ready = [&]() {
            if(counter != 0) {
                char data[20] = "\0foobar";
                Buffer pak {data, 7};
                //pak.convert<uint8_t>(0) = counter;
                pak.data[0] = counter;
                auto pak_buf = AllocBuffer::copy(pak);

                if(a.send(1000000 - counter, pak_buf)) {
                    counter --;
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
                cout << "read: " << *ret << endl;
            }
        };

        reactor.run();
    }
}
