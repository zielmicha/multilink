#include "libreactor/reactor.h"
#include "libreactor/future.h"

Future<unit> write_all(Reactor& reactor, StreamPtr out, Buffer data);
void read_all(StreamPtr in, std::function<void(Buffer)> data);
