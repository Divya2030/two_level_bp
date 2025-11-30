#ifndef BP_PREDICTORS_HPP
#define BP_PREDICTORS_HPP

#include <cstdint>
#include <unordered_map>

#include "types.hpp"
#include "automaton.hpp"

namespace bp {

/**
 * AlwaysTakenPredictor:
 *
 * A trivial static predictor which always predicts "taken".
 * Used as a baseline, similar to the "Always Taken" scheme
 * evaluated in the paper.
 */
class AlwaysTakenPredictor {
public:
    bool predict(std::uint64_t /*pc*/) const { return true; }
    void update(std::uint64_t /*pc*/, Outcome /*o*/) {}
};

/**
 * Bimodal2BitPredictor:
 *
 * Per-branch 2-bit saturating counter, indexed by PC.
 *
 * This is similar to a classic "bimodal" or "BTB-style" predictor
 * using the A2 automaton:
 *   - Each static branch has its own 2-bit counter.
 *   - Counter is incremented/decremented on taken/not-taken.
 *   - States 2 and 3 predict taken; 0 and 1 predict not-taken.
 *
 * This serves as a dynamic baseline for comparison to Two-Level AT.
 */
class Bimodal2BitPredictor {
public:
    bool predict(std::uint64_t pc) const {
        auto it = table_.find(pc);
        std::uint8_t st = (it == table_.end()) ? 3 : it->second; // default strongly taken
        return automaton_predict(AutomatonType::A2, st);
    }

    void update(std::uint64_t pc, Outcome o) {
        auto it = table_.find(pc);
        std::uint8_t st = (it == table_.end()) ? 3 : it->second;
        st = automaton_next(AutomatonType::A2, st, o);
        table_[pc] = st;
    }

private:
    std::unordered_map<std::uint64_t, std::uint8_t> table_; // pc → 2-bit state
};

} // namespace bp

#endif // BP_PREDICTORS_HPP
