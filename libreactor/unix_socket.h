#include <string>
#include "future.h"

namespace UnixSocket {
    void listen(Reactor& reactor, std::string path, function<void(FD*)> callback);
}
