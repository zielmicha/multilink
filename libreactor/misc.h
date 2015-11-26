#ifndef MISC_H_
#define MISC_H_
#include <vector>
#include "libreactor/reactor.h"

std::vector<FD*> fd_pair(Reactor& reactor);

void set_recv_buffer(FD* fd, int size);

std::string random_hex_string(int length);
std::string hex_decode(std::string s);

#endif
