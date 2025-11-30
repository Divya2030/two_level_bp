#ifndef BP_STATS_HPP
#define BP_STATS_HPP

#include <cstdint>

namespace bp {

/**
 * Statistics structure for counting predictions.
 *
 * For each predictor (AT variant or baseline), we track:
 *   - total   : number of dynamic branches seen
 *   - correct : number of correct predictions
 *
 * The accuracy is defined exactly as in the paper:
 *   accuracy = correct / total
 */
struct Stats {
    std::uint64_t total   = 0;
    std::uint64_t correct = 0;

    double accuracy() const {
        if (total == 0) return 0.0;
        return static_cast<double>(correct) / static_cast<double>(total);
    }
};

} // namespace bp

#endif // BP_STATS_HPP
