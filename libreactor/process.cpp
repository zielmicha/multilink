#include "process.h"
#include "signals.h"
#include "common.h"
#define LOGGER_NAME "process"
#include "logging.h"

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <assert.h>

Popen::Popen(Reactor& reactor): reactor(reactor) {
    target_fds[0] = 0;
    target_fds[1] = 1;
    target_fds[2] = 2;
}

Popen::Popen(Reactor& reactor, std::string executable): Popen(reactor) {
    arg(executable);
}

Popen::Popen(Reactor& reactor, std::vector<std::string> args): Popen(reactor) {
    this->args(args);
}

Popen& Popen::arg(std::string s) {
    arguments.push_back(s);
    return *this;
}

Popen& Popen::args(std::vector<std::string> vec) {
    for(auto item: vec)
        arg(item);
    return *this;
}

Popen& Popen::pipe(ProcessFile::Type fd) {
    target_fds[(int)fd] = -1;
    return *this;
}

std::shared_ptr<Process> Popen::exec() {
    auto ptr = std::shared_ptr<Process>(new Process(*this));
    Process::processes[ptr->pid] = ptr;
    return ptr;
}

void Popen::call(std::function<void(int)> callback) {
    exec()->wait(callback);
}

Future<int> Popen::call() {
    Completer<int> r;
    call(r.result_fn());
    return r.future();
}

Future<unit> Popen::check_call() {
    return call().then<unit>([](int ret) -> unit {
            if(ret != 0)
                throw CalledProcessError(ret);
            return unit();
        });
}

int Process::start_process(const Popen& options) {
    std::vector<const char*> argv;
    for(std::string item: options.arguments)
        argv.push_back(item.c_str());
    argv.push_back(nullptr);

    int pid = fork();
    if(pid == 0) {
        // Child
        try {
            sigset_t sigset;
            sigemptyset(&sigset);
            sigprocmask(SIG_SETMASK, &sigset, NULL);

            for(int i=0; i<3; i++)
                dup2(options.target_fds[i], i);

            execvp(argv[0], (char**)(&(*argv.begin())));
            perror("execvp");
            _exit(1);
        } catch(...) {
            fprintf(::stderr, "exception in child");
            _exit(1);
        }
    }
    return pid;
}

std::array<FD*, 3> Popen::init_pipe_fds() {
    std::array<FD*, 3> out;
    bool isread[3] = {true, false, false};
    for(int i=0; i<3; i++) {
        if(target_fds[i] == -1) {
            int pipefd[2];
            if(::pipe(pipefd) == -1)
                errno_to_exception();
            if(isread[i])
                std::swap(pipefd[0], pipefd[1]);
            target_fds[i] = pipefd[1];
            out[i] = &reactor.take_fd(pipefd[0]);
        } else {
            out[i] = nullptr;
        }
    }
    return out;
}

Process::Process(Popen options) {
    auto fds = options.init_pipe_fds();
    pid = Process::start_process(options);
    for(int i=0; i<3; i++)
        if(fds[i] != NULL)
            close(options.target_fds[i]);
    stdin = fds[0];
    stdout = fds[1];
    stderr = fds[2];
}

void Process::wait(std::function<void(int)> callback) {
    assert(initialized);
    if(finished)
        callback(exit_code);
    else
        on_finish.push_back(callback);
}

void Process::sigchld_handler() {
    while(true) {
        int status;
        int pid = waitpid((pid_t)(-1), &status, WNOHANG);
        if(pid < 0) break;
        auto iter = processes.find(pid);
        if(iter != processes.end()) {
            auto& proc = iter->second;
            proc->finished = true;
            proc->exit_code = WEXITSTATUS(status);

            for(auto fun: proc->on_finish)
                fun(proc->exit_code);
        }
    }
}

void Process::init() {
    initialized = true;
    Signals::register_signal_handler(SIGCHLD, Process::sigchld_handler);
}

bool Process::initialized = false;
decltype(Process::processes) Process::processes;
