#include <system_error>
#include <errno.h>

void errno_to_exception() {
    throw std::system_error(errno, std::system_category());
}
