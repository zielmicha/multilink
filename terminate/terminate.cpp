#include "terminate/terminate.h"

Terminator::Terminator(Reactor& reactor) {

}

TerminatorPtr Terminator::create(Reactor& reactor) {

}

Future<std::shared_ptr<PacketStream> > Terminator::create_target(uint64_t id) {

}

void Terminator::set_transport(TransportPtr transport) {

}

void Terminator::set_tun(TunPtr tun) {

}
