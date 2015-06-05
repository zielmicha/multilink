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

    BandwidthEstimator::BandwidthEstimator()
        : overall(1000) {}

    //const uint64_t BANDWIDTH_MEASURE_MIN_SIZE = 128;
    const uint64_t BURST_TIMEOUT = 60 * 1000;
    const int TRANSMIT_KEEP = 400;

    void BandwidthEstimator::data_written(uint64_t time, uint64_t bytes) {
        if(burst_running || time < burst_ends_at) {
            burst_ends_at = true;

            transmitted.push_back({time, bytes});
            transmit_size += bytes;

            if(transmitted.size() > TRANSMIT_KEEP) {
                transmit_size -= transmitted.front().second;
                transmitted.pop_front();
            }

            if(transmitted.size() < TRANSMIT_KEEP) {
                return;
            }

            uint64_t prev_trasmit = transmitted.front().first;
            uint64_t delta = time - prev_trasmit;

            overall.add_and_remove_back(transmit_size * 1000 / delta);

            DEBUG("delta " << delta << " bytes " << transmit_size <<
                  " current " << bandwidth_mbps());
        } else {
            if(transmit_size != 0)
                LOG("burst ended");
            transmit_size = 0;
            transmitted.clear();
        }
    }

    void BandwidthEstimator::write_ready(uint64_t time) {
    }

    void BandwidthEstimator::output_queue_full(uint64_t time) {
        LOG("output_queue_full");
        burst_running = true;
    }

    void BandwidthEstimator::input_queue_empty(uint64_t time) {
        DEBUG("input_queue_empty");
        if(burst_running) {
            burst_running = false;
            burst_ends_at = time + BURST_TIMEOUT;
        }
    }

    double BandwidthEstimator::bandwidth_mbps() {
        return std::max(overall.mean() / 1000, 0.1); // TODO
    }
}
