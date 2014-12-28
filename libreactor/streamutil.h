#include "reactor.h"
#include "future.h"

Future<unit> write_all(Reactor& reactor, Stream* out, Buffer data);
void read_all(Stream* in, std::function<void(Buffer)> data);