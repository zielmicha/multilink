#include "logging.h"
#include "tcp.h"

int main() {
    Reactor reactor;
    TCP::connect(reactor, "127.0.0.1", 8001).on_success([](FD* fd) {
            LOG("connected");
            fd->on_read_ready = [fd]() {
                while(true) {
                    char buff[10];
                    auto size = fd->read(buff, sizeof(buff));
                    std::cout << url_encode(buff) << std::endl;
                    std::cout << "read bytes=" << size << std::endl;
                    if(size == 0) break;
                }
            };
        });
    reactor.run();
}
