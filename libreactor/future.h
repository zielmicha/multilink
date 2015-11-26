#ifndef FUTURE_H_
#define FUTURE_H_
#include "libreactor/reactor.h"
#include <cassert>

struct unit {};

enum class FutureState {
    WAITING,
    IMMEDIATE_EXCEPTION,
    IMMEDIATE_VALUE
};

int printf(const char* format, ...);

template <typename T>
struct _BaseFutureData {
    virtual T get_value() = 0;
    virtual ~_BaseFutureData() {}

    FutureState state;
    std::unique_ptr<std::exception> exception;
    std::function<void(T)> value_callback;
    std::function<void(std::unique_ptr<std::exception>)> exception_callback;
};

template <typename T>
struct _NoCast {
    T cast(const T& t) {
        return t;
    }
    typedef T Target;
};

void _future_nop();

template <typename T, typename CASTER = _NoCast<T> >
struct _FutureData : public _BaseFutureData<typename CASTER::Target> {
    T value;

    _FutureData() {}
    _FutureData(T&& value): value(std::move(value)) {}
    _FutureData(const T& value): value(value) {}

    typename CASTER::Target get_value() {
        return CASTER().cast(value);
    }
};

template <typename T>
struct FutureMaybeUnwrap {
    // if T is Future<X> return X else return T
    typedef T type;
};

template <typename T>
class Future {
    std::shared_ptr<_BaseFutureData<T> > data;

public:
    explicit Future(std::shared_ptr<_BaseFutureData<T> > data): data(data) {}

#ifdef ENABLE_FUTURE_CAST
    // Think if it is really safe (i.e. different _BaseFutureDatas has same layouts)
    std::shared_ptr<void> _get_data() { return std::static_pointer_cast<void>(data); }

    template <typename R,
              typename = std::enable_if<std::is_convertible<R, T>::value > >
    Future(Future<R> other): data(
        std::static_pointer_cast<_BaseFutureData<T> >(other._get_data())) {}
#endif

    static Future<T> make_immediate(T value) {
        auto dataptr = new _FutureData<T>;
        dataptr->state = FutureState::IMMEDIATE_VALUE;
        dataptr->value = value;

        return Future(std::shared_ptr<_BaseFutureData<T> >(dataptr));
    }

    void on_success_or_failure(
        const std::function<void(T)>& fun_value,
        const std::function<void(std::unique_ptr<std::exception>)>& fun_exc) const {
        switch(data->state) {
        case FutureState::IMMEDIATE_VALUE:
            fun_value(data->get_value());
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
    Future<R> then(std::function<Future<R>(T)> fun) const;

    template <typename R>
    Future<R> then(std::function<R(T)> fun) const;

    /** Automatically deduce X in then<X>(...) */
    template <typename F,
              typename RetType = typename FutureMaybeUnwrap<
                  decltype(std::declval<F>()(std::declval<T>()))>::type >
    auto then(F fun) const -> Future<RetType> {
        return then<RetType>(fun);
    }

    bool has_result() const {
        return data->state != FutureState::WAITING;
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
struct FutureMaybeUnwrap<Future<T> > {
    typedef T type;
};

template <typename T, typename CASTER = _NoCast<T> >
class _BaseCompleter {
protected:
    std::shared_ptr<_FutureData<T, CASTER> > data = nullptr;

    void init() {
        auto data_ptr = &(*this->data); // avoid cyclic reference

        data->exception_callback = [data_ptr](std::unique_ptr<std::exception> ex) {
            data_ptr->state = FutureState::IMMEDIATE_EXCEPTION;
            data_ptr->exception = std::move(ex);
        };
    }
public:

    void result_failure(std::unique_ptr<std::exception> ex) const {
        assert(data->state == FutureState::WAITING);
        data->state = FutureState::IMMEDIATE_EXCEPTION;
        data->exception_callback(std::move(ex));
    }

    typedef Future<typename CASTER::Target> FutureType;

    FutureType future() const {
        return FutureType(data);
    }
};

template <typename T, typename CASTER = _NoCast<T> >
class Completer : public _BaseCompleter<T, CASTER> {
public:
    Completer() {
        this->data = std::make_shared<_FutureData<T, CASTER> >();
        this->data->state = FutureState::WAITING;

        auto data_ptr = &(*this->data); // avoid cyclic reference
        this->data->value_callback = [data_ptr](T value) {
            data_ptr->state = FutureState::IMMEDIATE_VALUE;
            data_ptr->value = value;
        };
        this->init();
    }

    Completer(const Completer& c) {
        this->data = c.data;
    }

    void result(const T& ret) const {
        assert(this->data->state == FutureState::WAITING);
        this->data->state = FutureState::IMMEDIATE_VALUE;
        this->data->value_callback(ret);
    }

    std::function<void(T)> result_fn() const {
        Completer self = *this;
        return [self](T ret) {
            self.result(ret);
        };
    }
};

template <typename T>
template <typename R>
Future<R> Future<T>::then(std::function<Future<R>(T)> fun) const {
    Completer<R> f;
    on_success_or_failure(
        [f, fun](T val) {
            Future<R> ret = fun(val);
            ret.on_success(f.result_fn());
        },
        [f, fun](std::unique_ptr<std::exception> ex) {
            f.result_failure(std::move(ex));
        });
    return f.future();
}

template <typename T>
template <typename R>
Future<R> Future<T>::then(std::function<R(T)> fun) const {
    Completer<R> f;
    on_success_or_failure(
        [=](T val) {
            R ret = fun(val);
            f.result(ret);
        },
        [=](std::unique_ptr<std::exception> ex) {
            f.result_failure(std::move(ex));
        });
    return f.future();
}

template <typename T, typename CASTER = _NoCast<T> >
class ImmediateCompleter : public _BaseCompleter<T, CASTER> {
public:
    ImmediateCompleter(T&& value) {
        this->data = std::shared_ptr<_FutureData<T, CASTER> >(
            new _FutureData<T, CASTER>(std::move(value)));
        this->data->state = FutureState::WAITING;
        this->data->value_callback = [](typename CASTER::Target target) {};
        this->init();
    }

    ImmediateCompleter(const ImmediateCompleter& other) {
        this->data = other.data;
    }

    T& value() const {
        return this->data->value;
    }

    typename CASTER::Target cast_value() const {
        return CASTER().cast(value());
    }

    void result() const {
        assert(this->data->state == FutureState::WAITING);
        this->data->state = FutureState::IMMEDIATE_VALUE;
        this->data->value_callback(cast_value());
    }
};

template <typename T>
Future<T> make_future(const T& val) {
    return Future<T>::make_immediate(val);
}

#endif
