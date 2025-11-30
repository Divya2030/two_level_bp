#ifndef BP_HRT_HPP
#define BP_HRT_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace bp {

/**
 * HRTKind corresponds exactly to Section 3.1:
 *
 * - IHRT: Ideal History Register Table
 *         * One entry per static conditional branch.
 *         * No capacity limit or interference.
 *
 * - AHRT: Associative History Register Table
 *         * Implemented as N-entry, W-way set-associative cache.
 *         * Lower bits of PC index the set, higher bits form a tag.
 *         * Uses round-robin for replacement (simplified LRU).
 *
 * - HHRT: Hash History Register Table
 *         * Implemented as a fixed-size direct-mapped table.
 *         * Index is a hash of PC (we use simple PC>>2 & mask).
 *         * No tags, so collisions cause history interference.
 */
enum class HRTKind {
    AHRT,
    HHRT,
    IHRT
};

/**
 * HistoryTable is the abstract interface for the first-level structure
 * in Fig. 1 (History Register Table).
 *
 * Each entry is:
 *   - A k-bit shift register holding the recent history (R_{i,c-k+1} ... R_{i,c}).
 */
class HistoryTable {
public:
    virtual ~HistoryTable() = default;

    /**
     * Get the current k-bit history for the branch at PC.
     * If the branch has not been seen before, the implementation should
     * return a reasonable default (paper uses all 1s to bias to taken).
     */
    virtual std::uint16_t get(std::uint64_t pc) = 0;

    /**
     * Set the current k-bit history for the branch at PC.
     */
    virtual void set(std::uint64_t pc, std::uint16_t history) = 0;

    /**
     * Capacity in entries (for approximate hardware cost calculation).
     * For IHRT, this is the current number of distinct PCs.
     * For AHRT/HHRT, it is the fixed table size.
     */
    virtual std::size_t capacity_entries() const = 0;
};

/**
 * IHRT: Ideal History Register Table.
 *
 * - Implemented with an std::unordered_map<PC, history>.
 * - Conceptually infinite capacity (limited only by memory).
 * - Used to model the upper bound on AT performance with no interference.
 */
class IHRTTable : public HistoryTable {
public:
    explicit IHRTTable(int history_bits);

    std::uint16_t get(std::uint64_t pc) override;
    void set(std::uint64_t pc, std::uint16_t history) override;
    std::size_t capacity_entries() const override;

private:
    int history_bits_;
    std::uint16_t init_history_;
    std::unordered_map<std::uint64_t, std::uint16_t> table_;
};

/**
 * HHRT: Hash History Register Table.
 *
 * - Implemented as a simple array indexed by a hash of PC.
 * - No tag stored → collisions lead to history reuse / interference.
 * - Represents a low-cost, but somewhat less accurate, design.
 */
class HHRTTable : public HistoryTable {
public:
    HHRTTable(int entries, int history_bits);

    std::uint16_t get(std::uint64_t pc) override;
    void set(std::uint64_t pc, std::uint16_t history) override;
    std::size_t capacity_entries() const override;

private:
    int entries_;
    std::uint32_t mask_;
    std::uint16_t init_history_;
    std::vector<std::uint16_t> hist_;

    std::uint32_t index(std::uint64_t pc) const;
};

/**
 * AHRT: Associative History Register Table.
 *
 * - Implemented as a W-way set-associative cache with "entries" total lines.
 * - Each line:
 *     * valid bit
 *     * tag
 *     * k-bit history register
 * - On miss:
 *     * Choose a victim way via round-robin (simplified LRU).
 *     * IMPORTANT: We do NOT reinitialize the history register when
 *                  we reassign a line to a new PC, which preserves the
 *                  interference behavior described by the paper.
 */
class AHRTTable : public HistoryTable {
public:
    AHRTTable(int entries, int ways, int history_bits);

    std::uint16_t get(std::uint64_t pc) override;
    void set(std::uint64_t pc, std::uint16_t history) override;
    std::size_t capacity_entries() const override;

private:
    struct Entry {
        bool          valid;
        std::uint32_t tag;
        std::uint16_t history;
    };

    int entries_;          // total number of lines (e.g., 512)
    int ways_;             // associativity (e.g., 4)
    int sets_;             // entries_ / ways_
    std::uint16_t init_history_;
    int set_index_bits_;   // log2(sets_)

    std::vector<std::vector<Entry>> table_; // table_[set][way]
    std::vector<int>                next_victim_; // round-robin pointer per set

    std::uint32_t set_index(std::uint64_t pc) const;
    std::uint32_t tag_for(std::uint64_t pc) const;
    Entry& access(std::uint64_t pc);
};

} // namespace bp

#endif // BP_HRT_HPP
