#include "reactor.h"
#include <cassert>

struct unit {};

enum class FutureState {
    WAITING,
    IMMEDIATE_EXCEPTION,
    IMMEDIATE_VALUE
};

int printf(const char* format, ...);

template <typename T>
struct _FutureData {
    FutureState state;
    T value;
    std::unique_ptr<std::exception> exception;
    std::function<void(T)> value_callback;
    std::function<void(std::unique_ptr<std::exception>)> exception_callback;
};

void _future_nop();

template <typename T>
class Future {
    std::shared_ptr<_FutureData<T> > data;

    Future(std::shared_ptr<_FutureData<T> > data): data(data) {}
public:

    Future() {
        data = std::shared_ptr<_FutureData<T> >(new _FutureData<T>);
        data->state = FutureState::WAITING;

        auto data_ptr = &(*data); // avoid cyclic reference
        data->value_callback = [data_ptr](T value) {
            data_ptr->state = FutureState::IMMEDIATE_VALUE;
            data_ptr->value = value;
        };
        data->exception_callback = [data_ptr](std::unique_ptr<std::exception> ex) {
            data_ptr->state = FutureState::IMMEDIATE_EXCEPTION;
            data_ptr->exception = std::move(ex);
        };
    }

    Future(T value) {
        data = std::shared_ptr<_FutureData<T> >(new _FutureData<T>);
        data->state = FutureState::IMMEDIATE_VALUE;
        data->value = value;
    }

    void on_success_or_failure(
        const std::function<void(T)>& fun_value,
        const std::function<void(std::unique_ptr<std::exception>)>& fun_exc) const {
        switch(data->state) {
        case FutureState::IMMEDIATE_VALUE:
            fun_value(data->value);
            break;
        case FutureState::IMMEDIATE_EXCEPTION:
            fun_exc(std::move(data->exception));
            break;
        case FutureState::WAITING:
            data->value_callback = fun_value;
            data->exception_callback = fun_exc;
            break;
        }
    }

    void on_success(const std::function<void(T)>& fun_value) const {
        on_success_or_failure(
            fun_value,
            [](std::unique_ptr<std::exception> ex) {
                throw *ex;
            });
    }

    template <typename R>
    Future<R> then(std::function<Future<R>(T)> fun) const {
        Future<R> f;
        on_success_or_failure(
            [f, fun](T val) {
                Future<R> ret = fun(val);
                ret.on_success(f.result_fn());
            },
            [f, fun](std::unique_ptr<std::exception> ex) {
                f.result_failure(std::move(ex));
            });
        return f;
    }

    template <typename R>
    Future<R> then(std::function<R(T)> fun) const {
        Future<R> f;
        on_success_or_failure(
            [=](T val) {
                R ret = fun(val);
                f.result(ret);
            },
            [=](std::unique_ptr<std::exception> ex) {
                f.result_failure(ex);
            });
        return f;
    }

    // result being const is counter-intuitive, but
    // Future is just a wrapper for _FutureData
    void result(const T& ret) const {
        assert(data->state == FutureState::WAITING);
        data->state = FutureState::IMMEDIATE_VALUE;
        data->value_callback(ret);
    }

    void result_failure(std::unique_ptr<std::exception> ex) const {
        assert(data->state == FutureState::WAITING);
        data->state = FutureState::IMMEDIATE_EXCEPTION;
        data->exception_callback(std::move(ex));
    }

    bool has_result() const {
        return data->state != FutureState::WAITING;
    }

    std::function<void(T)> result_fn() const {
        auto data1 = data;
        return [data1](T ret) {
            Future self(data1);
            self.result(ret);
        };
    }

    T wait(Reactor& reactor) const {
        bool ready = false;
        T value;
        on_success([&](T val) {
                ready = true;
                value = val;
            });
        while(!ready) reactor.step();
        return value;
    }

    void ignore() const {
    }
};

template <typename T>
Future<T> make_future(const T& val) {
    return Future<T>(val);
}
