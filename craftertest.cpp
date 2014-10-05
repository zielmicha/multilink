#include "craftertun.h"
#include "process.h"

int main() {
    Reactor reactor;
    Tun tun(reactor, "toto%d");

    reactor.run();
}
