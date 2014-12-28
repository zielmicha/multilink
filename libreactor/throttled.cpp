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

DelayedStream::DelayedStream(Reactor& reactor, Stream* stream,
                             uint64_t buffsize, uint64_t delay): reactor(reactor),
                                                                 timer(reactor),
                                                                 stream(stream),
                                                                 delay(delay),
                                                                 buffsize(buffsize),
                                                                 on_error(nothing) {
    stream->set_on_read_ready(std::bind(&DelayedStream::read_ready, this));
    stream->set_on_write_ready(std::bind(&DelayedStream::write_ready, this));
    stream->set_on_error(std::bind(&DelayedStream::error, this));
}

Buffer DelayedStream::read(Buffer data) {
    return stream->read(data);
}

void DelayedStream::close() {
    stream->close();
    stream = NULL;
}

size_t DelayedStream::write(Buffer data) {
    size_t wrote = std::min(data.size, buffsize);
    buffsize -= wrote;
    buffers.emplace_back(wrote);
    data.slice(0, wrote).copy_to(buffers.back().as_buffer());

    timer.once(delay, [this]() {
        waiting ++;
        write_ready();
    });

    return wrote;
}

void DelayedStream::write_ready() {
    if(waiting == 0)
        return;

    Buffer buff = buffers.front().as_buffer();

    size_t wrote = stream->write(buff.slice(front_pointer));

    if(wrote + front_pointer == buff.size) {
        buffsize += buff.size;
        buffers.pop_front();
        waiting --;
        front_pointer = 0;
    } else {
        front_pointer += wrote;
    }

    if(buffsize != 0)
        on_write_ready();
}

void DelayedStream::read_ready() {
    on_read_ready();
}

void DelayedStream::error() {
    on_error();
}