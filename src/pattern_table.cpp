#include "pattern_table.hpp"

namespace bp {

/**
 * Construct a PT with 2^history_bits entries.
 *
 * Each entry is initialized based on the automaton type:
 *   - LastTime → state = 1  (predict taken at start)
 *   - A2/A3/A4 → state = 3  (strongly taken)
 *
 * This follows Section 4.2 of the paper.
 */
PatternTable::PatternTable(int history_bits, AutomatonType automaton)
    : history_bits_(history_bits),
      mask_((1u << history_bits) - 1u),
      automaton_(automaton),
      entries_(1u << history_bits, automaton_init_state(automaton))
{}

bool PatternTable::predict(std::uint16_t history) const {
    std::uint32_t idx = history & mask_;
    return automaton_predict(automaton_, entries_[idx]);
}

void PatternTable::update(std::uint16_t history, Outcome o) {
    std::uint32_t idx = history & mask_;
    std::uint8_t& st  = entries_[idx];
    st = automaton_next(automaton_, st, o);
}

} // namespace bp
