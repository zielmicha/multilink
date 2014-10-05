#ifndef CRAFTERTUN_H_
#define CRAFTERTUN_H_

#include <string>

#include "craftertun.h"
#include "common.h"
#include "reactor.h"

class Tun {
protected:
    Reactor& reactor;
    FD* fd;
public:
    Tun(Reactor& r, std::string name);
    Tun(const Tun& t) = delete;

    std::string name;
};

#endif
