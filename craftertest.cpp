#include "craftertun.h"

int main() {
    Reactor reactor;
    Tun tun(reactor, "toto%d");


    reactor.run();
}
