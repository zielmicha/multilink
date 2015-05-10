#ifndef SIGNALS_H_
#define SIGNALS_H_
#include "function.h"

namespace Signals {
    void init();
    void register_signal_handler(int sig, function<void()> fun);
    void call_handlers();
}

#endif
