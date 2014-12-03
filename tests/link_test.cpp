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
        Link b {reactor, fds[1]};

        int counter = 10;

        a.on_send_ready = [&]() {
            cout << "a.on_send_ready counter=" << counter << endl;

            if(counter != 0) {
                char data[20] = "\0foobar";
                Buffer pak {data, 7};
                //pak.convert<uint8_t>(0) = counter;
                pak.data[0] = counter;
                LOG("write " << pak);

                if(a.send(pak)) {
                    counter --;
                }
            }
        };

        a.on_recv_ready = []() {
            cout << "a.on_recv_ready" << endl;
        };

        b.on_send_ready = []() {
            //cout << "b.on_send_ready" << endl;
        };

        b.on_recv_ready = [&]() {
            while(true) {
                cout << "b.on_recv_ready" << endl;
                optional<Buffer> ret = b.recv();
                if(!ret) return;
                cout << "read: " << *ret << endl;
            }
        };

        cout << "a.send" << endl;

        reactor.run();
    }
}
