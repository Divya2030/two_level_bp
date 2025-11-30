#ifndef BP_TWO_LEVEL_AT_HPP
#define BP_TWO_LEVEL_AT_HPP

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

#include "at_config.hpp"
#include "pattern_table.hpp"
#include "hrt.hpp"
#include "types.hpp"

namespace bp {

/**
 * TwoLevelATPredictor:
 *
 * Implements AT(HRT(kSR), PT(2^k,Automaton)) from:
 *   - Section 2: Concept of Two-Level Adaptive Training
 *   - Section 2.1: History registers and pattern table
 *
 * Operation for each dynamic branch at PC:
 *
 *   1. Get local history:
 *        H_i = HRT.get(pc)
 *
 *   2. Prediction:
 *        z_c = A( S_c(H_i) ) = PT.predict(H_i)
 *
 *   3. After the branch is resolved:
 *        S_{c+1} = δ(S_c, R_{i,c})      → PT.update(H_i, outcome)
 *        H_i'    = (H_i << 1) | bit     → HRT.set(pc, H_i')
 */
class TwoLevelATPredictor {
public:
    explicit TwoLevelATPredictor(const ATConfig& cfg);

    const std::string& name() const { return name_; }

    // Predict branch at PC using HRT+PT.
    bool predict(std::uint64_t pc);

    // Update HRT & PT with actual outcome.
    void update(std::uint64_t pc, Outcome o);

    /**
     * Approximate hardware cost in bits.
     *
     * This is not an exact transistor count, but a simple metric:
     *   - HRT cost = (#HRT entries) * history_bits
     *   - PT cost  = (#PT entries) * 2   (assuming 2-bit FSMs)
     */
    std::size_t hardware_cost_bits() const;

private:
    std::string              name_;
    int                      history_bits_;
    std::uint32_t            mask_;
    PatternTable             pt_;
    std::unique_ptr<HistoryTable> hrt_;
};

} // namespace bp

#endif // BP_TWO_LEVEL_AT_HPP
