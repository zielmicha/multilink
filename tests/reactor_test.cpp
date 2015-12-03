#include "libreactor/reactor.h"
#include <iostream>

int main() {
    Reactor reactor;
    FDPtr stdin = reactor.take_fd(0);
    //FD& stdout = reactor.take_fd(1);

    stdin->on_read_ready = [&]() {
        std::cout << "on_read_ready" << std::endl;
        while(true) {
            StackBuffer<10> buff;
            Buffer data = stdin->read(buff);
            std::cout << "read bytes=" << data.size << std::endl;
            if(data.size == 0) break;
        }
    };

    reactor.run();
}
