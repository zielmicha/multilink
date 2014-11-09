#include "common.h"
#include <system_error>
#include <errno.h>

std::unique_ptr<std::exception> errno_get_exception() {
    return std::unique_ptr<std::exception>(new std::system_error(errno, std::system_category()));
}

void errno_to_exception() {
    throw *errno_get_exception();
}
