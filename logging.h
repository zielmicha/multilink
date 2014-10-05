#ifndef LOGGING_H_
#define LOGGING_H_
#include <iostream>
#include <string>

#ifndef LOGGER_NAME
#define LOGGER_NAME "default"
#endif

#ifndef NO_LOG
#define LOG(args...) std::cerr << LOGGER_NAME << ": " << args << std::endl;
#else
#define NO_DEBUG
#define LOG(args...)
#endif

#ifndef NO_DEBUG
#define DEBUG LOG
#else
#define DEBUG(args..)
#endif

void setup_crash_handlers();
std::string url_encode(const std::string &value);

#endif
