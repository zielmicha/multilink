#ifndef MULTILINK_STATS_H
#define MULTILINK_STATS_H
#include <deque>
#include <cstdint>

namespace Multilink {
    class Stats {
        std::deque<int> values;
        int count;

        long long sum = 0;
        long long sum_of_squares = 0;

        void remove();
        void add(int val);
    public:
        Stats(int count = 10);
        void add_and_remove_back(int val);
        double mean();
        double stddev();
    };

    class BandwidthEstimator {
        int64_t sampling_time;

    public:
        BandwidthEstimator(int64_t sampling_time);
        void data_transmitted(int64_t time, int64_t bytes);
        int64_t bandwidth();
    };
}
#endif
