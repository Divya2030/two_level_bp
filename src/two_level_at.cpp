#include "two_level_at.hpp"

namespace bp {

/**
 * Construct a Two-Level AT predictor based on an ATConfig.
 *
 * We:
 *   1. Create the appropriate HRT (IHRT/AHRT/HHRT).
 *   2. Create a PatternTable with 2^k entries and the chosen automaton.
 */
TwoLevelATPredictor::TwoLevelATPredictor(const ATConfig& cfg)
    : name_(cfg.name),
      history_bits_(cfg.history_bits),
      mask_((1u << cfg.history_bits) - 1u),
      pt_(cfg.history_bits, cfg.automaton)
{
    switch (cfg.hrt_kind) {
        case HRTKind::IHRT:
            hrt_ = std::make_unique<IHRTTable>(cfg.history_bits);
            break;

        case HRTKind::AHRT:
            hrt_ = std::make_unique<AHRTTable>(
                cfg.hrt_entries, cfg.hrt_ways, cfg.history_bits);
            break;

        case HRTKind::HHRT:
            hrt_ = std::make_unique<HHRTTable>(
                cfg.hrt_entries, cfg.history_bits);
            break;
    }
}

/**
 * Predict branch at PC:
 *   1. Look up k-bit history from HRT.
 *   2. Use that history to index the PT and predict via A(S_c).
 */
bool TwoLevelATPredictor::predict(std::uint64_t pc) {
    std::uint16_t h = hrt_->get(pc);
    return pt_.predict(h);
}

/**
 * Update predictor after the actual outcome is known:
 *
 *   1. old_h = HRT.get(pc)
 *   2. PT.update(old_h, outcome)   // δ(S_c, R_{i,c})
 *   3. new_h = (old_h << 1 | bit) & mask_
 *   4. HRT.set(pc, new_h)
 */
void TwoLevelATPredictor::update(std::uint64_t pc, Outcome o) {
    std::uint16_t old_h = hrt_->get(pc);

    // Update pattern table using old history pattern.
    pt_.update(old_h, o);

    // Shift in the newest branch result into the k-bit history register.
    std::uint16_t new_h = static_cast<std::uint16_t>(
        ((old_h << 1) | (o == Outcome::Taken ? 1u : 0u)) & mask_);
    hrt_->set(pc, new_h);
}

/**
 * Approximate hardware cost in bits:
 *
 *   HRT bits = (#entries in HRT) * history_bits
 *   PT bits  = (#entries in PT)  * 2 (2-bit counters)
 *
 * Useful for generating plots of "accuracy vs hardware cost".
 */
std::size_t TwoLevelATPredictor::hardware_cost_bits() const {
    std::size_t hrt_bits = hrt_->capacity_entries() * history_bits_;
    std::size_t pt_bits  = pt_.num_entries() * 2u;
    return hrt_bits + pt_bits;
}

} // namespace bp
