#include "libreactor/reactor.h"
#include "libreactor/future.h"

Future<unit> write_all(Reactor& reactor, Stream* out, Buffer data);
void read_all(Stream* in, std::function<void(Buffer)> data);
