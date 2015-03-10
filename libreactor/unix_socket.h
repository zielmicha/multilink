#include <string>
#include "future.h"

namespace UnixSocket {
    void listen(Reactor& reactor, std::string path, std::function<void(FD*)> callback);
}
