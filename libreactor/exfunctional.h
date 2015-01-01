#ifndef EXFUNCTIONAL_H_
#define EXFUNCTIONAL_H_
#include <vector>

template <class A, class B, class C>
std::vector<C> mapvector(const A& src, std::function<C(B)> func) {
    std::vector<C> ret;
    ret.reserve(src.size());
    for(const auto& item: src)
        ret.push_back(func(item));

    return ret;
}

#endif
