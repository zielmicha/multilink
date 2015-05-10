#include "function.h"

void func(int arg) {}

int main() {
    static_assert(StackFunction::PerformCall<decltype(func), void, int>::value == true, "");
}
