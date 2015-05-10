#ifndef FUNCTION_H_
#define FUNCTION_H_
#include <cassert>
#include <functional>
#include <iostream>
#include <new>
#include <utility>
#include <array>
#include <memory>
#include <type_traits>

namespace StackFunction {

    template<typename T> using Invoke = typename T::type;
    template <typename Condition, typename T = void> using EnableIf = Invoke<std::enable_if<Condition::value, T>>;
    template <typename Condition, typename T = void> using DisableIf = Invoke<std::enable_if<!Condition::value, T>>;
    template <typename T> using Decay = Invoke<std::remove_const<Invoke<std::remove_reference<T>>>>;
    template <typename T, typename U> struct IsRelated : std::is_same<Decay<T>, Decay<U>> {};

    template <bool B>
    using bool_constant = std::integral_constant<bool, B>;

    template<typename Signature>
    struct Function;

    template <typename F, typename R, typename... Args>
    struct PerformCall {
    };

    template <typename From, typename To>
    struct IsConvertible : std::is_convertible<From, To> {};

    template <typename From>
    struct IsConvertible<From, void> : bool_constant<true> {};

    template <typename F, typename R>
    struct PerformCall<F, R> : IsConvertible<
        decltype((std::declval<F>())()), R> {};

    template <typename F, typename R, typename Arg>
    struct PerformCall<F, R, Arg> : IsConvertible<
        decltype((std::declval<F>())(std::declval<Arg>())), R> {};

    template <typename F, typename R, typename Arg1, typename Arg2>
    struct PerformCall<F, R, Arg1, Arg2> : IsConvertible<
        decltype((std::declval<F>())(std::declval<Arg1>(), std::declval<Arg2>())), R> {};

    inline void count_() {
        static int counter = 0;
        counter ++;
        if(counter > 1000) abort();
    }

template<typename R, typename... Args>
struct Function<R(Args...)>
{
    Function() : mImpl() {}
    template<typename F,
             typename = DisableIf<IsRelated<F, Function>>,
             typename = EnableIf<PerformCall<F, R, Args...> > >
        Function(F f) : mImpl(std::make_shared<Impl<F>>(std::move(f))) {}

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    Function(const Function&) noexcept = default;
    Function& operator=(const Function&) noexcept = default;

    R operator()(Args ...args) const {
        if (!mImpl) {
            throw std::bad_function_call();
        }
        return mImpl->call(std::forward<Args>(args)...);
    }

private:
    struct ImplBase {
        virtual ~ImplBase() {}
        virtual R call(Args ...args) = 0;
    };

    template<typename F>
    struct Impl : ImplBase {
        template<typename FArg>
        Impl(FArg&& f) : f(std::forward<FArg>(f)) {
            #ifdef CHECK_FUNC_ALLOC
            count_();
            #endif
        }

        ~Impl() {}

        virtual R call(Args... args) {
            return f(std::forward<Args>(args)...);
        }

        F f;
    };

    std::shared_ptr<ImplBase> mImpl;
};
}

template <typename Sig>
using function = StackFunction::Function<Sig>;

#endif
