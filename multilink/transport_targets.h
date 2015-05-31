#ifndef TRANSPROT_TARGETS_H_
#define TRANSPROT_TARGETS_H_
#include "transport.h"
#include "tcp.h"

TargetCreator create_connecting_tcp_target_creator(Reactor& reactor, std::string host, int port);
void create_listening_tcp_target_creator(Reactor& reactor, std::shared_ptr<Transport> transport, std::string host, int port);
TargetCreator unknown_stream_target_creator();

#endif
