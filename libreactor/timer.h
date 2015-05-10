#ifndef TIMER_H_
#define TIMER_H_
#include <queue>
#include "reactor.h"

class Timer {
    FD* fd;

    struct Event {
        uint64_t time;
        function<void()> func;

        Event(uint64_t time, const function<void()>& func): time(time), func(func) {};

        bool operator<(const Event& other) const {
            return time > other.time;
        }
    };

    std::priority_queue<Event> queue;

    void timer_ready();
    void schedule_front();
    void schedule_for(uint64_t time);

public:
    Timer(Reactor& r);
    ~Timer();

    void schedule(uint64_t time, const function<void()> &func);
    void once(uint64_t delta, const function<void()>& func);
    static uint64_t get_time();
};

#endif
