#include <stdlib.h>
#include "process.h"
#include "logging.h"

int main() {
    setup_crash_handlers();
    Process::init();
    Reactor reactor;
    Popen(reactor, "ls").arg("-al").arg("/").call([&](int code) {
            LOG("process exited with code " << code);
            reactor.exit();
        });
    reactor.run();
    LOG("finished");
}
