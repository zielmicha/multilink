#ifndef PROCESS_H_
#define PROCESS_H_
#include <memory>
#include "reactor.h"
#include "future.h"
#include <unordered_map>
#include <vector>

class Process;

class ProcessFile {
public:
    enum Type {
        STDIN = 0,
        STDOUT = 1,
        STDERR = 2
    };
};

struct CalledProcessError: public std::runtime_error {
    CalledProcessError(int exit_code) :
        runtime_error("Process exited with exit code " + exit_code),
        exit_code(exit_code) {};

    int exit_code;
};

class Popen {
    friend class Process;
    Reactor& reactor;
    std::vector<std::string> arguments;
    std::string executable;
    int target_fds[3];

    std::array<FD*, 3> init_pipe_fds();
public:
    Popen(Reactor& reactor, std::string executable);

    std::shared_ptr<Process> exec();
    void call(std::function<void(int)> callback);
    Future<int> call();
    Future<unit> check_call();
    int call_blocking();

    Popen& arg(std::string arg);
    Popen& args(std::vector<std::string> args);
    Popen& pipe(ProcessFile::Type fd);
};

class Process {
    static int start_process(const Popen& options);
    static void sigchld_handler();
    static bool initialized;
    static std::unordered_map<int, std::shared_ptr<Process> > processes;

    std::vector<std::function<void(int)> > on_finish;
    bool finished;
    int exit_code;

    friend class Popen;
    Process(Popen options);
public:
    static void init();

    Process(const Process& other) = delete;

    void wait(std::function<void(int)> callback);

    int pid;
    FD* stdin;
    FD* stdout;
    FD* stderr;
};

#endif
