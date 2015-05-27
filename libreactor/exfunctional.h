#ifndef EXFUNCTIONAL_H_
#define EXFUNCTIONAL_H_
#include <vector>
#include <functional>

template <class Col, class Func>
auto mapvector(const Col& src, Func func) -> std::vector<decltype(func(src[0]))> {
    std::vector<decltype(func(src[0]))> ret;
    ret.reserve(src.size());
    for(const auto& item: src)
        ret.push_back(func(item));

    return ret;
}

#endif
