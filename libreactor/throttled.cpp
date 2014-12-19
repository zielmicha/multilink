#include "throttled.h"
#include "misc.h"
#include "logging.h"

ThrottledStream::ThrottledStream(Reactor& reactor, Stream* stream, double mbps)
    : reactor(reactor),
      timer(reactor),
      stream(stream),
      mbps(mbps),
      on_error(nothing) {
    reactor.schedule(std::bind(&ThrottledStream::write_ready, this));
    stream->set_on_read_ready(std::bind(&ThrottledStream::read_ready, this));
    stream->set_on_write_ready(std::bind(&ThrottledStream::write_ready, this));
    stream->set_on_error(std::bind(&ThrottledStream::error, this));
}

Buffer ThrottledStream::read(Buffer data) {
    return stream->read(data);
}

size_t ThrottledStream::write(const Buffer data) {
    if(next_transmission <= Timer::get_time()) {
        size_t wrote = stream->write(data);
        uint64_t elapse_time = wrote / mbps;

        next_transmission = Timer::get_time() + elapse_time;
        timer.schedule(next_transmission, std::bind(&ThrottledStream::write_ready, this));
        return wrote;
    }
    return 0;
}

void ThrottledStream::close() {
    stream->close();
    stream = NULL;
}

void ThrottledStream::write_ready() {
    on_write_ready();
}

void ThrottledStream::read_ready() {
    on_read_ready();
}

void ThrottledStream::error() {
    on_error();
}
