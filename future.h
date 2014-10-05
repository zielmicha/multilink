#include "reactor.h"
#include <cassert>

struct unit {};

template <typename T>
struct _FutureData {
    bool immediate;
    T value;
    std::function<void(T)> callback;
};

void _future_nop();

template <typename T>
class Future {
    std::shared_ptr<_FutureData<T> > data;

    Future(std::shared_ptr<_FutureData<T> > data): data(data) {}
public:

    Future() {
        data = std::shared_ptr<_FutureData<T> >(new _FutureData<T>);
        data->immediate = false;
    }

    Future(T value) {
        data = std::shared_ptr<_FutureData<T> >(new _FutureData<T>);
        data->immediate = true;
        data->value = value;
    }

    void on_success(std::function<void(T)> fun) {
        if(data->immediate)
            fun(data->value);
        else
            data->callback = fun;
    }

    template <typename R>
    Future<R> then(std::function<Future<R>(T)> fun) {
        Future<R> f;
        on_success([f, fun](T val) {
                Future<R> f1 = f;
                Future<R> ret = fun(val);
                ret.on_success(f1.result_fn());
            });
        return f;
    }

    template <typename R>
    Future<R> then(std::function<R(T)> fun) {
        Future<R> f;
        on_success([=](T val) {
                R ret = fun(val);
                f.result(ret);
            });
        return f;
    }

    void result(const T& ret) {
        assert(!data->immediate);
        data->callback(ret);
    }

    std::function<void(T)> result_fn() {
        auto data1 = data;
        return [data1](T ret) {
            Future self(data1);
            self.result(ret);
        };
    }

    T wait(Reactor& reactor) {
        bool ready = false;
        T value;
        on_success([&](T val) {
                ready = true;
                value = val;
            });
        while(!ready) reactor.step();
        return value;
    }
};

template <typename T>
Future<T> make_future(const T& val) {
    return Future<T>(val);
}
