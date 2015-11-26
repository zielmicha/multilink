#include "libreactor/logging.h"
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

static void handler(int sig) {
  void *array[10];
  size_t size;

  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void print_stack_trace() {
  void *array[10];
  size_t size;

  size = backtrace(array, 10);

  fprintf(stderr, "Print stack trace:\n");
  backtrace_symbols_fd(array, size, STDERR_FILENO);
}

void setup_crash_handlers() {
  signal(SIGSEGV, handler);
  signal(SIGINT, handler);
  signal(SIGABRT, handler);
}

std::string current_time() {
    timeval c_time;
    gettimeofday(&c_time, NULL);
    int milli = c_time.tv_usec / 1000;

    char buffer1[80];
    strftime(buffer1, sizeof(buffer1), "%H:%M:%S", localtime(&c_time.tv_sec));

    char buffer2[90];
    sprintf(buffer2, "%s.%03d", buffer1, milli);

    return buffer2;
}

std::string url_encode(std::string value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for(auto c: value) {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << '%' << std::setw(2) << int((unsigned char) c);
    }

    return escaped.str();
}
