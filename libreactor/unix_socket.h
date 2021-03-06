#include <string>
#include "libreactor/future.h"

namespace UnixSocket {
    void listen(Reactor& reactor, std::string path, std::function<void(FDPtr)> callback);
    StreamPtr connect(Reactor& reactor, std::string path);
    FDPtr bind(Reactor& reactor, std::string path);
    void listen(Reactor& reactor, FDPtr socket, std::function<void(FDPtr)> accept_cb);

    Future<unit> send_fd(Reactor& reactor, FDPtr socket, int fd);
    Future<int> recv_fd(Reactor& reactor, FDPtr socket);
}
