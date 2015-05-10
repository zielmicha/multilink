#ifndef TCP_H_
#define TCP_H_
#include "reactor.h"
#include "future.h"
#include <string>

namespace TCP {
    Future<FD*> connect(Reactor& reactor, std::string addr, int port);
    Future<unit> listen(Reactor& reactor, std::string addr, int port, function<void(FD*)> accept);
}

#endif
