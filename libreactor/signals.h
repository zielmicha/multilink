namespace Signals {
    void init();
    void register_signal_handler(int sig, std::function<void()> fun);
    void call_handlers();
}
