#ifndef BP_TYPES_HPP
#define BP_TYPES_HPP

#include <cstdint>

namespace bp {

/**
 * Outcome of a single dynamic branch.
 *
 * This directly corresponds to R_{i,c} in Section 2.1 of the paper:
 *   - R_{i,c} = 1  → branch taken
 *   - R_{i,c} = 0  → branch not taken
 *
 * We use a strongly-typed enum to avoid confusion with raw integers.
 */
enum class Outcome : std::uint8_t {
    NotTaken = 0,
    Taken    = 1
};

} // namespace bp

#endif // BP_TYPES_HPP
