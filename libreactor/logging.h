#ifndef LOGGING_H_
#define LOGGING_H_
#include <iostream>
#include <string>

#ifndef LOGGER_NAME
#define LOGGER_NAME "default"
#endif

#ifndef NO_LOG
#define LOG(args...) std::cerr << current_time() << " " << LOGGER_NAME << ": " << args << std::endl
#else
#define NO_DEBUG
#define LOG(args...)
#endif

#ifdef LOG_DEBUG
#define DEBUG LOG
#else
#define DEBUG(args...)
#endif

#define ERROR(args...) LOG(args)

std::string current_time();
void setup_crash_handlers();
std::string url_encode(std::string value);
void print_stack_trace();

#endif
