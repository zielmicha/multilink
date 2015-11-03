#include "logging.h"
#include <cstring>

extern "C" void app_main(int length, const char* path);

int main(int argc, char** args) {
    setup_crash_handlers();
    auto path = argc == 2 ? args[1] : "app.sock";
    app_main(strlen(path), path);
}
