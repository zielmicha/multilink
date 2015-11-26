#include "libreactor/timer.h"
#include "libreactor/common.h"
#include <time.h>
#include <sys/timerfd.h>

Timer::Timer(Reactor& reactor) {
    int ffd = timerfd_create(CLOCK_MONOTONIC, 0);
    fd = &reactor.take_fd(ffd);
    fd->on_read_ready = std::bind(&Timer::timer_ready, this);
}

void Timer::timer_ready() {
    StackBuffer<sizeof(uint64_t)> buff;
    fd->read(buff);

    uint64_t current = Timer::get_time();
    while(!queue.empty() && queue.top().time <= current) {
        const Event& ev = queue.top();
        ev.func();
        queue.pop();
    }

    if(!queue.empty())
        schedule_front();
}

void Timer::schedule_front() {
    schedule_for(queue.top().time);
}

void Timer::schedule_for(uint64_t time) {
    struct itimerspec spec = {{0, 0}, {0, 0}};
    spec.it_value.tv_sec = time / 1000000;
    spec.it_value.tv_nsec = (time % 1000000) * 1000;
    timerfd_settime(fd->fileno(), TFD_TIMER_ABSTIME, &spec, NULL);
}

Timer::~Timer() {
    fd->close();
}

void Timer::schedule(uint64_t time, const std::function<void()>& func) {
    queue.emplace(time, func);
    schedule_front();
}

void Timer::once(uint64_t delta, const std::function<void()>& func) {
    schedule(get_time() + delta, func);
}

uint64_t Timer::get_time() {
    struct timespec spec;
    if(clock_gettime(CLOCK_MONOTONIC, &spec) != 0)
        errno_to_exception();
    return spec.tv_sec * 1000000ull + spec.tv_nsec / 1000ull;
}
