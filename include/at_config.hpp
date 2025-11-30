#ifndef BP_AT_CONFIG_HPP
#define BP_AT_CONFIG_HPP

#include <string>
#include "hrt.hpp"
#include "automaton.hpp"

namespace bp {

/**
 * ATConfig encodes a single AT scheme in the style of Table 2 of the paper:
 *
 *   AT( HRT(size, kSR), PT(2^k, Automaton) )
 *
 * where:
 *   - name        : human-readable label for printing & CSV
 *   - hrt_kind    : IHRT / AHRT / HHRT
 *   - hrt_entries : number of HRT entries (for AHRT/HHRT)
 *   - hrt_ways    : associativity (for AHRT)
 *   - history_bits: k (length of history shift register)
 *   - automaton   : Last-Time, A2, A3, or A4
 */
struct ATConfig {
    std::string   name;
    HRTKind       hrt_kind;
    int           hrt_entries;   // for AHRT/HHRT
    int           hrt_ways;      // for AHRT
    int           history_bits;  // k
    AutomatonType automaton;
};

} // namespace bp

#endif // BP_AT_CONFIG_HPP
