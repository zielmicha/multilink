#include "timer.h"
#include "logging.h"

int main() {
    Reactor reactor;
    Timer timer {reactor};

    for(int i=10; i >= 0; i--) {
        timer.once(i * 1000000, [i, &reactor, &timer]() {
            LOG("tick " << i);
            if(i == 10) reactor.exit();
            timer.once(500000, [] {
                LOG("halftick");
            });
        });
    }

    reactor.run();
}
