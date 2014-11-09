#ifndef COMMON_H_
#define COMMON_H_
#include <memory>

std::unique_ptr<std::exception> errno_get_exception();
void errno_to_exception() __attribute__((noreturn));

#endif
