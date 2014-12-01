#include "multilink.h"
#include "misc.h"
#include "buffer.h"
using namespace Multilink;
using namespace std;

int main() {
    Reactor reactor;
    vector<FD*> fds = fd_pair(reactor);

    {
        Link a {reactor, fds[0]};
        Link b {reactor, fds[1]};

        StackBuffer<4096> buff;

        a.on_send_ready = []() {
            cout << "a.on_send_ready" << endl;
        };

        a.on_recv_ready = []() {
            cout << "a.on_recv_ready" << endl;
        };

        b.on_send_ready = []() {
            cout << "b.on_send_ready" << endl;
        };

        b.on_recv_ready = [&]() {
            while(true) {
                cout << "b.on_recv_ready" << endl;
                optional<Buffer> ret = b.recv(buff);
                if(!ret) return;
                cout << "read: " << ret << endl;
            }
        };

        cout << "a.send" << endl;
        a.send(Buffer((char*)"\0\x08" "foobar", 8));

        reactor.run();
    }
}
