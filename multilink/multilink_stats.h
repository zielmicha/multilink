#ifndef MULTILINK_STATS_H
#define MULTILINK_STATS_H
#include <deque>

namespace Multilink {
    class Stats {
        std::deque<int> values;
        int count;

        long long sum = 0;
        long long sum_of_squares = 0;

        void remove();
        void add(int val);
    public:
        Stats(int count);
        void add_and_remove_back(int val);
        double mean();
        double stddev();
    };
}
#endif
