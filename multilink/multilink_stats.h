#ifndef MULTILINK_STATS_H
#define MULTILINK_STATS_H
#include <deque>
#include <cstdint>

const uint64_t MBPS_TO_BPS = 1000 * 1000;

namespace Multilink {
    class Stats {
        std::deque<long long> values;
        int count;

        long long sum = 0;
        long long sum_of_squares = 0;

        void remove();
        void add(long long val);
    public:
        Stats(int count = 10);
        void add_and_remove_back(long long val);
        double mean();
        double stddev();
    };

    class BandwidthEstimator {
        Stats stats;
        uint64_t transmit_start = 0;
        uint64_t transmit_size = 0;

    public:
        BandwidthEstimator();
        void data_written(uint64_t time, uint64_t bytes);
        void write_ready(uint64_t time);

        double bandwidth_mbps();
    };
}
#endif
