#include "multilink.h"

const int64_t SAMPLING_TIME = 2000;

namespace Multilink {
    Link::Link(Stream* stream): stream(stream), bandwidth(SAMPLING_TIME) {}
}
