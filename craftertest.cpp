#include "craftertun.h"
#include "process.h"
#include "logging.h"

int main() {
    setup_crash_handlers();
    Process::init();
    Reactor reactor;
    Tun tun(reactor, "toto%d");
    tun.on_recv = [](Crafter::Packet& p) {
        p.Print();
    };

    Popen(reactor, "true").check_call().wait(reactor);
    Popen(reactor, {"ifconfig", tun.name, "10.10.0.1/24", "up"}).check_call().wait(reactor);

    reactor.run();
}
