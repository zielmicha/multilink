#ifndef SIGNALS_H_
#define SIGNALS_H_
#include <signal.h>
#include <functional>

namespace Signals {
    void init();
    void register_signal_handler(int sig, std::function<void()> fun);
    void call_handlers();
    void ignore_signal(int sig);
}

#endif
