#ifndef TCP_H_
#define TCP_H_
#include "libreactor/reactor.h"
#include "libreactor/future.h"
#include <string>

namespace TCP {
    Future<FD*> connect(Reactor& reactor, std::string addr, int port, std::string bind = "");
    Future<unit> listen(Reactor& reactor, std::string addr, int port, std::function<void(FD*)> accept);
}

#endif
