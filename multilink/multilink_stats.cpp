#include "multilink_stats.h"
#include "logging.h"
#include <cmath>

namespace Multilink {
    Stats::Stats(int count): count(count) {
    }

    void Stats::remove() {
        long long val = values.front();
        values.pop_front();
        sum_of_squares -= ((long long)val) * val;
        sum -= val;
    }

    void Stats::add(long long val) {
        values.push_back(val);
        sum_of_squares += ((long long)val) * val;
        sum += val;
    }

    double Stats::mean() {
        if(values.empty()) return 0;
        return sum / (double)values.size();
    }

    double Stats::stddev() {
        if(values.empty()) return 0;
        // s^2 = sqrt(1/n \sum_i (a_i-m)^2)
        // ns^2 = \sum_i (a_i-m)^2
        // ns^2 = \sum_i a_i^2 - 2a_im + m^2
        // ns^2 = (\sum_i a_i^2) + 2m(\sum_i a_i) + nm^2
        // ns^2 = (\sum_i a_i^2) - 2nm^2 + nm^2
        // s^2 = (\sum_i a_i^2)/n - m^2
        double m = mean();
        return sqrt(sum_of_squares / (double)values.size() - ((double)m)*m);
    }

    void Stats::add_and_remove_back(long long val) {
        if((int)values.size() >= count)
            remove();
        add(val);
    }

    BandwidthEstimator::BandwidthEstimator():
        stats(100) {

    }

    void BandwidthEstimator::data_written(uint64_t time, uint64_t bytes) {
        if(transmit_size == 0) {
            transmit_start = time;
        }
        transmit_size += bytes;
    }

    const uint64_t BANDWIDTH_MEASURE_MIN_SIZE = 128;

    void BandwidthEstimator::write_ready(uint64_t time) {
        if(transmit_size < BANDWIDTH_MEASURE_MIN_SIZE)
            return;
        uint64_t delta = time - transmit_start;
        if(delta == 0) delta = 1;
        uint64_t bps = (transmit_size * MBPS_TO_BPS / delta);
        //LOG("measure " << transmit_size << " " << delta);
        stats.add_and_remove_back(bps);
        transmit_start = 0;
        transmit_size = 0;
    }

    double BandwidthEstimator::bandwidth_mbps() {
        return stats.mean() / MBPS_TO_BPS;
    }
}
