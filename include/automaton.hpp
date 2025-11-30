#ifndef BP_AUTOMATON_HPP
#define BP_AUTOMATON_HPP

#include <cstdint>
#include "types.hpp"

namespace bp {

/**
 * AutomatonType corresponds to the finite-state machines in Fig. 2:
 *
 *   - LastTime: stores only the last outcome (1 bit).
 *   - A2      : 2-bit saturating up/down counter.
 *   - A3, A4  : similar style counters with more states; in this project we
 *               implement them identically to A2 but keep them as distinct
 *               enum values so that the code structure matches the paper.
 *
 * The paper's notation:
 *   - S_c   : pattern history bits ("state")
 *   - A(S_c): prediction decision function
 *   - δ     : state transition function
 */
enum class AutomatonType {
    LastTime,
    A2,
    A3,
    A4
};

/**
 * Initial state S_0 for each automaton.
 *
 * Section 4.2 says:
 *   - For A1/A2/A3/A4, initialize to state 3 (strongly taken).
 *   - For Last-Time, initialize to predict taken (state = 1).
 */
inline std::uint8_t automaton_init_state(AutomatonType t) {
    switch (t) {
        case AutomatonType::LastTime:
            // Last outcome = Taken
            return 1;
        case AutomatonType::A2:
        case AutomatonType::A3:
        case AutomatonType::A4:
        default:
            // Strongly taken (2-bit counter = 3)
            return 3;
    }
}

/**
 * Prediction function A(S_c).
 *
 * Returns:
 *   true  → predict Taken
 *   false → predict Not taken
 *
 * For Last-Time:
 *   - state is just the last outcome bit.
 *
 * For A2/A3/A4 (2-bit counter style):
 *   - states 2 and 3 predict Taken
 *   - states 0 and 1 predict Not taken
 */
inline bool automaton_predict(AutomatonType t, std::uint8_t state) {
    switch (t) {
        case AutomatonType::LastTime:
            return (state & 1u) != 0;
        case AutomatonType::A2:
        case AutomatonType::A3:
        case AutomatonType::A4:
        default:
            return state >= 2;
    }
}

/**
 * State transition δ(S_c, R_{i,c}).
 *
 * For Last-Time:
 *   - overwrite with the last outcome's bit.
 *
 * For A2/A3/A4:
 *   - standard saturating up/down counter:
 *     * Taken     → increment (up to max 3)
 *     * NotTaken → decrement (down to min 0)
 */
inline std::uint8_t automaton_next(AutomatonType t,
                                   std::uint8_t state,
                                   Outcome o) {
    switch (t) {
        case AutomatonType::LastTime:
            return (o == Outcome::Taken) ? 1 : 0;

        case AutomatonType::A2:
        case AutomatonType::A3:
        case AutomatonType::A4:
        default:
            if (o == Outcome::Taken) {
                if (state < 3) ++state;
            } else {
                if (state > 0) --state;
            }
            return state;
    }
}

} // namespace bp

#endif // BP_AUTOMATON_HPP
