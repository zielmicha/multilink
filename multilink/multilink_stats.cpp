#include "multilink_stats.h"
#include <cmath>

namespace Multilink {
    void Stats::remove() {
        int val = values.front();
        values.pop_front();
        sum_of_squares -= ((long long)val) * val;
        sum -= val;
    }

    void Stats::add(int val) {
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

    void Stats::add_and_remove_back(int val) {
        if((int)values.size() >= count)
            remove();
        add(val);
    }
}
