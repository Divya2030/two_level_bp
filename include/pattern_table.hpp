#ifndef BP_PATTERN_TABLE_HPP
#define BP_PATTERN_TABLE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

#include "automaton.hpp"

namespace bp {

/**
 * PatternTable (PT) – second-level table in Fig. 1.
 *
 * size = 2^k entries, where k = history_bits.
 *
 * Each entry corresponds to one possible k-bit history pattern and stores
 * the pattern history bits S_c in the form of a finite-state machine state
 * (Last-Time, A2, A3, A4).
 *
 * - predict(history):
 *     * uses the automaton's A(S_c) to predict taken/not-taken.
 *
 * - update(history, outcome):
 *     * calls δ(S_c, R_{i,c}) to move to the new state.
 */
class PatternTable {
public:
    PatternTable(int history_bits, AutomatonType automaton);

    // Predict next outcome based on current history pattern.
    bool predict(std::uint16_t history) const;

    // Update pattern entry with the actual outcome.
    void update(std::uint16_t history, Outcome o);

    std::size_t num_entries() const { return entries_.size(); }

private:
    int history_bits_;
    std::uint32_t mask_;
    AutomatonType automaton_;
    std::vector<std::uint8_t> entries_; // pattern history bits S_c
};

} // namespace bp

#endif // BP_PATTERN_TABLE_HPP
