#ifndef MISC_H_
#define MISC_H_
#include <vector>
#include "reactor.h"

std::vector<FD*> fd_pair(Reactor& reactor);
inline void nothing() {};

void set_recv_buffer(FD* fd, int size);

#endif
