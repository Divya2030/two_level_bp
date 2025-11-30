#include "hrt.hpp"

namespace bp {

// ======================= IHRTTable =======================

IHRTTable::IHRTTable(int history_bits)
    : history_bits_(history_bits),
      init_history_((1u << history_bits) - 1u) // all 1s -> Taken bias
{}

/**
 * Get the current history for PC.
 * If PC has not been seen before, return all 1s as in Section 4.2
 * (bias initial state to Taken).
 */
std::uint16_t IHRTTable::get(std::uint64_t pc) {
    auto it = table_.find(pc);
    if (it == table_.end()) return init_history_;
    return it->second;
}

/**
 * Store the updated history for PC.
 */
void IHRTTable::set(std::uint64_t pc, std::uint16_t history) {
    table_[pc] = history;
}

/**
 * For cost estimation, capacity is the number of distinct static branches.
 */
std::size_t IHRTTable::capacity_entries() const {
    return table_.size();
}

// ======================= HHRTTable =======================

HHRTTable::HHRTTable(int entries, int history_bits)
    : entries_(entries),
      mask_(static_cast<std::uint32_t>(entries - 1)),
      init_history_((1u << history_bits) - 1u),
      hist_(entries, init_history_)
{}

/**
 * Compute index into the hash table from PC.
 * We drop 2 LSBs (word alignment) and AND with (entries-1).
 */
std::uint32_t HHRTTable::index(std::uint64_t pc) const {
    return static_cast<std::uint32_t>((pc >> 2) & mask_);
}

/**
 * Read the history from the hashed slot.
 */
std::uint16_t HHRTTable::get(std::uint64_t pc) {
    return hist_[index(pc)];
}

/**
 * Write the history into the hashed slot.
 * Note: collisions are not checked; this is the intended behavior to
 * emulate hash collisions and interference.
 */
void HHRTTable::set(std::uint64_t pc, std::uint16_t history) {
    hist_[index(pc)] = history;
}

std::size_t HHRTTable::capacity_entries() const {
    return static_cast<std::size_t>(entries_);
}

// ======================= AHRTTable =======================

AHRTTable::AHRTTable(int entries, int ways, int history_bits)
    : entries_(entries),
      ways_(ways),
      sets_(entries / ways),
      init_history_((1u << history_bits) - 1u),
      set_index_bits_(0)
{
    // Compute log2(sets_)
    while ((1 << set_index_bits_) < sets_) {
        ++set_index_bits_;
    }

    table_.resize(sets_, std::vector<Entry>(ways_));
    next_victim_.assign(sets_, 0);

    // Initialize all entries as invalid with history = all 1s.
    for (int s = 0; s < sets_; ++s) {
        for (int w = 0; w < ways_; ++w) {
            table_[s][w].valid   = false;
            table_[s][w].tag     = 0;
            table_[s][w].history = init_history_;
        }
    }
}

/**
 * Compute which set a PC maps to (lower bits of PC after dropping 2 LSBs).
 */
std::uint32_t AHRTTable::set_index(std::uint64_t pc) const {
    return static_cast<std::uint32_t>((pc >> 2) & (sets_ - 1));
}

/**
 * Compute tag from higher-order bits of PC.
 */
std::uint32_t AHRTTable::tag_for(std::uint64_t pc) const {
    return static_cast<std::uint32_t>(pc >> (2 + set_index_bits_));
}

/**
 * Access the entry for PC:
 *   - On hit, returns the matching line.
 *   - On miss, chooses a victim via round-robin and returns that line.
 *
 * IMPORTANT: on miss, we mark the victim as valid and set its tag, but we
 * DO NOT reset its history to the initial state; this preserves
 * "interference" as described in Section 3.1.
 */
AHRTTable::Entry& AHRTTable::access(std::uint64_t pc) {
    std::uint32_t si  = set_index(pc);
    std::uint32_t tag = tag_for(pc);
    auto& set         = table_[si];

    // Check all ways in this set for a hit.
    for (int w = 0; w < ways_; ++w) {
        if (set[w].valid && set[w].tag == tag) {
            return set[w];
        }
    }

    // Miss: choose a victim via round-robin.
    int victim = next_victim_[si];
    next_victim_[si] = (victim + 1) % ways_;

    Entry& e = set[victim];
    e.valid = true;
    e.tag   = tag;
    // e.history left unchanged intentionally.

    return e;
}

std::uint16_t AHRTTable::get(std::uint64_t pc) {
    Entry& e = access(pc);
    return e.history;
}

void AHRTTable::set(std::uint64_t pc, std::uint16_t history) {
    Entry& e = access(pc);
    e.history = history;
}

std::size_t AHRTTable::capacity_entries() const {
    return static_cast<std::size_t>(entries_);
}

} // namespace bp
