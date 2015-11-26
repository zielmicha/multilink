#ifndef TERMINATE_TUN_H_
#define TERMINATE_TUN_H_

#include <string>

#include "libreactor/common.h"
#include "libreactor/reactor.h"

class Tun {
protected:
    Reactor& reactor;
    FD* fd;

    void on_read();
public:
    Tun(Reactor& r, std::string name);
    Tun(const Tun& t) = delete;

    std::function<void(Buffer)> on_recv;
    std::string name;
};

typedef std::shared_ptr<Tun> TunPtr;

#endif
