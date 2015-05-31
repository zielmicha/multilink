#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <functional>
#include <unordered_map>
#define LOGGER_NAME "signals"
#include "logging.h"

namespace Signals {
    sigset_t sigset;
    bool signal_appeared[64];
    std::unordered_map<int, std::function<void()> > signal_handlers;
    std::vector<int> signals_registered;
    bool initialized;

    void init() {
        if(initialized) return;
        initialized = true;
        sigemptyset(&sigset);
    }

    void handle(int sig) {
        signal_appeared[sig] = true;
    }

    void register_signal_handler(int sig, std::function<void()> fun) {
        init();

        signal_appeared[sig] = false;
        signal_handlers[sig] = fun;
        signals_registered.push_back(sig);

        sigaddset(&sigset, sig);
        sigprocmask(SIG_SETMASK, &sigset, NULL);

        struct sigaction sa;
        sa.sa_handler = &handle;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        if(sigaction(SIGCHLD, &sa, 0) == -1) {
            perror("sigaction");
            exit(1);
        }
    }

    void call_handlers() {
        for(int sig: signals_registered) {
            if(signal_appeared[sig]) {
                signal_appeared[sig] = false;
                signal_handlers[sig]();
            }
        }
    }
}
