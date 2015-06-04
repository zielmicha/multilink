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
        long long get_sum() { return sum; }
    };

    class BandwidthEstimator {
        Stats overall;
        std::deque<std::pair<uint64_t, uint64_t> > transmitted;

        uint64_t transmit_size = 0;
        uint64_t burst_ends_at = 0;
        bool burst_running = false;

    public:
        BandwidthEstimator();
        void data_written(uint64_t time, uint64_t bytes);
        void write_ready(uint64_t time);
        void output_queue_full(uint64_t time);
        void input_queue_empty(uint64_t time);

        double bandwidth_mbps();
    };
}
#endif
