#include <string>
#include "future.h"

namespace UnixSocket {
    void listen(Reactor& reactor, std::string path, std::function<void(FD*)> callback);
    Future<unit> send_fd(Reactor& reactor, FD* socket, int fd);
    Future<int> recv_fd(Reactor& reactor, FD* socket);
}
