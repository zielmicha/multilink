#include "libreactor/future.h"

int main() {
    Future<int> f = Future<int>::make_immediate(0);

    Future<int> g = f.then([](int v) { return v + 1; });
    Future<int> h = f.then([](int v) { return Future<int>::make_immediate(v + 1); });
}
