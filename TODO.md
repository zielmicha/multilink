
* better transport congestion control (SSH-like)
* refactor Stream* to std::shared_ptr<Stream>
  * typedef std::shared_ptr<Stream> StreamPtr?
  * typedef std::shared_ptr<PacketStream> PacketStreamPtr?
* '#include "reactor.h"' -> '#include <libreactor/reactor.h>'
