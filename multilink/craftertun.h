#ifndef CRAFTERTUN_H_
#define CRAFTERTUN_H_

#include <string>

#include "craftertun.h"
#include "common.h"
#include "reactor.h"

#include <crafter.h>

class Tun {
protected:
    Reactor& reactor;
    FD* fd;

    void on_read();
public:
    Tun(Reactor& r, std::string name);
    Tun(const Tun& t) = delete;

    function<void(Crafter::Packet&)> on_recv;
    std::string name;
};

#endif
