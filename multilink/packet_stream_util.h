#ifndef PACKET_STREAM_UTIL_H_
#define PACKET_STREAM_UTIL_H_
#include "packet_stream.h"
#include "reactor.h"

void pipe(Stream* in, PacketStream* out);
void pipe(PacketStream* in, Stream* out);

#endif
